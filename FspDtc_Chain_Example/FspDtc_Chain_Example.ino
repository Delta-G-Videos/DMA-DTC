#include "r_dtc.h"
#include "IRQManager.h"

#define ARRAY_SIZE 64

//  These are arrays to play with transferring in and out of
uint8_t source[ARRAY_SIZE];
uint8_t dest[ARRAY_SIZE];

// These four structs hold the configuration information needed by the HAL
dtc_instance_ctrl_t ctrl;                     // The HAL will initialize this in Open
transfer_info_t info[3];                         // This gets built in setup
dtc_extended_cfg_t extend_cfg;                // This gets built in setup
transfer_cfg_t cfg = { info, &extend_cfg };  // This just holds pointers to the previous two. 

volatile bool interruptFired = false;

void setup() {

  pinMode(2, INPUT_PULLUP);
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("\n\n*** FspDtc_Chain_Example.ino ***\n\n");

  // put some initial data in source (1 2 3 4 5 ...)
  for (int i = 0; i < ARRAY_SIZE; i++) {
    source[i] = i + 1;
  }  

  // Setup the transfer settings and configuration  
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  info[0].transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_REPEAT_AREA_DESTINATION - TRANSFER_REPEAT_AREA_SOURCE */
  info[0].transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  /* TRANSFER_IRQ_END - TRANSFER_IRQ_EACH */
  info[0].transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  /* TRANSFER_CHAIN_MODE_DISABLED - TRANSFER_CHAIN_MODE_EACH - TRANSFER_CHAIN_MODE_END */
  info[0].transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_EACH;    // CHAIN_MODE is DTC ONLY
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  info[0].transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_SIZE_1_BYTE - TRANSFER_SIZE_2_BYTE - TRANSFER_SIZE_4_BYTE */
  info[0].transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  /* TRANSFER_MODE_NORMAL - TRANSFER_MODE_REPEAT - TRANSFER_MODE_BLOCK - TRANSFER_MODE_REPEAT_BLOCK */
  info[0].transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;

  info[0].p_dest = dest;   // pointer to where data should go to
  info[0].p_src = source;  // pointer to where data should come from
  info[0].num_blocks = 0;  // unused in normal mode - number of repeats in repeat mode - number of blocks in block mode
  info[0].length = 3;        // number of transfers to make in normal mode - size of repeat area in repeat mode - size of block in block mode

  // Build settings for the other transfers.  
  info[1].transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  info[1].transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  info[1].transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  info[1].transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_EACH;
  info[1].transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  info[1].transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  info[1].transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
  info[1].p_dest = dest + 5;   // pointer to where data should go to
  info[1].p_src = source + 5;  // pointer to where data should come from
  info[1].num_blocks = 0;
  info[1].length = 3; 

  info[2].transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  info[2].transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  info[2].transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  info[2].transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
  info[2].transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  info[2].transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  info[2].transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
  info[2].p_dest = dest + 10;   // pointer to where data should go to
  info[2].p_src = source + 10;  // pointer to where data should come from
  info[2].num_blocks = 0;
  info[2].length = 3; 

  /* Trigger Source */
  /* vvvvvvvvvvvvvv */
  /***************************************************************************
     I'm using IRQ0 to trigger the transfers.
     Here you would set up whatever interrupt you want to use for that purpose.
     You must get the index to the vector table for the interrupt you want to use.
     IRQManager will give that to you in some cases.  
     In others you must search the IELSR registers
  ***************************************************************************/

  // Attach the IRQ0 interrupt with attachInterrupt
  // Then find the vector entry to give to the DTC extend_cfg
  attachInterrupt(digitalPinToInterrupt(2), irq0Callback, FALLING);
  // Find the entry in IELSR for IRQ0 and set as activation source
  for (int i=0; i<32; i++){
    if((R_ICU->IELSR[i] & 0xFF) == ELC_EVENT_ICU_IRQ0) {
      // Set the activation source for the DTC to the interrupt channel we found
      // The interrupt channel will match the vector table index. 
      extend_cfg.activation_source = (IRQn_Type)i;
    }
  }

  R_DTC_Open(&ctrl, &cfg);   // opens and configures the channel
  R_DTC_Enable(&ctrl);       // enable the channel for transfer
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
    /********************************************************
    // NOTE FOR CHAIN DTC: The reset function only resets the 1st transfer in the chain.
    //    You must manually reset the rest.
    ********************************************************/
    // This will put different values in for the second time, 
    // but after that it's just rewriting the same data to the same 
    // places so you won't see any differences after the second interrupt. 
    info[1].p_src = source + 5;
    info[1].p_dest = dest + 15;
    info[1].length = 3;
    info[2].p_src = source + 10;
    info[2].p_dest = dest + 20;
    info[2].length = 3;
    R_DTC_Reset(&ctrl, source + 30, dest, 5);  // restart a new batch of transfers.
  }
  delay(1000);
}

void irq0Callback(){
  interruptFired = true;
}