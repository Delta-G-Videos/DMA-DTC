#include "r_dmac.h"
#include "IRQManager.h"

#define ARRAY_SIZE 64

//  These are arrays to play with transferring in and out of
uint8_t source[ARRAY_SIZE];
uint8_t dest[ARRAY_SIZE];

// These four structs hold the configuration information needed by the HAL
dmac_instance_ctrl_t ctrl;                     // The HAL will initialize this in Open
transfer_info_t info;                         // This gets built in setup
dmac_extended_cfg_t extend_cfg;                // This gets built in setup
transfer_cfg_t cfg = { &info, &extend_cfg };  // This just holds pointers to the previous two. 

volatile bool interruptFired = false;

void setup() {

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("\n\n*** FspDma_Example.ino ***\n\n");

  // put some initial data in source (1 2 3 4 5 ...)
  for (int i = 0; i < ARRAY_SIZE; i++) {
    source[i] = i + 1;
  }  

  // Setup the transfer settings and configuration  
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_REPEAT_AREA_DESTINATION - TRANSFER_REPEAT_AREA_SOURCE */
  info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  /* TRANSFER_IRQ_END - TRANSFER_IRQ_EACH */
  info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  /* TRANSFER_CHAIN_MODE_DISABLED - TRANSFER_CHAIN_MODE_EACH - TRANSFER_CHAIN_MODE_END */
  info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;    // CHAIN_MODE is DTC ONLY
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_SIZE_1_BYTE - TRANSFER_SIZE_2_BYTE - TRANSFER_SIZE_4_BYTE */
  info.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  /* TRANSFER_MODE_NORMAL - TRANSFER_MODE_REPEAT - TRANSFER_MODE_BLOCK - TRANSFER_MODE_REPEAT_BLOCK */
  info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;

  info.p_dest = dest;   // pointer to where data should go to
  info.p_src = source;  // pointer to where data should come from
  info.num_blocks = 0;  // unused in normal mode - number of repeats in repeat mode - number of blocks in block mode
  info.length = 5;      // number of transfers to make in normal mode - size of repeat area in repeat mode - size of block in block mode

  extend_cfg.offset = 1;                 // offset size if using TRANSFER_ADDR_MODE_OFFSET
  extend_cfg.src_buffer_size = 1;        //  used for repeat - block mode
  extend_cfg.irq = FSP_INVALID_VECTOR;   //  IRQManager will set this
  extend_cfg.ipl = (BSP_IRQ_DISABLED);   //  IRQManager will set this
  extend_cfg.channel = 0;                //  IRQManager will set this
  extend_cfg.p_callback = dmacCallback;  // use NULL if not attaching an interrupt
  extend_cfg.p_context = NULL;           // void* pointer to anything will be available in callback
  extend_cfg.activation_source = ELC_EVENT_ICU_IRQ0;  // From Table 13.4 in RA4M1 Hardware User's Manual (use ELC_EVENT_NONE for software start)

  // Attaching the interrupt is optional but if you use it then 
  // it must be done before opening the channel
  IRQManager::getInstance().addDMA(extend_cfg);

  R_DMAC_Open(&ctrl, &cfg);   // opens and configures the channel
  R_DMAC_Enable(&ctrl);       // enable the channel for transfer

  /* Trigger Source */
  /* vvvvvvvvvvvvvv */

  // I'm using IRQ0 to trigger the transfers.
  // Here you would set up whatever interrupt you want to use for that purpose.
  // setup the interrupt register for IRQ0
  R_ICU->IRQCR[0] = 0xB0;  // Max filtering falling edge.
  // set port 1 pin 5 (digital pin 2 on Minima / digital pin 3 on R4-WiFi)
  // as input pullup and turn on interrupt
  R_PFS->PORT[1].PIN[5].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PCR_Pos) | (1 << R_PFS_PORT_PIN_PmnPFS_ISEL_Pos);
}

void loop() {
  static int counter = 0;
  Serial.print("  -");
  Serial.print(counter++);
  Serial.print("- Printing Dest ");
  for (int i = 0; i < ARRAY_SIZE; i++) {
    Serial.print(dest[i]);
    Serial.print(" ");
  }
  Serial.println();
  
  if(interruptFired){
    interruptFired = false;
    Serial.println("Interrupt Fired!");
    // Reset the data in the destination array
    for(int i=0; i<ARRAY_SIZE; i++){
      dest[i] = 0;
    }
    // The source and destination addresses can be NULL if you want to leave them where they are.
    // The last argment is the length for the new transfer.  
    // These settings will NOT be copied into the info struct.  
    R_DMAC_Reset(&ctrl, source, dest, 7);  // restart a new batch of transfers.
  }
  delay(1000);
}

// Callback function for DMA interrupt if used
void dmacCallback(dmac_callback_args_t * cb_data){
  // cb_data->p_context  will be the pointer set in the configuration
  interruptFired = true;
}