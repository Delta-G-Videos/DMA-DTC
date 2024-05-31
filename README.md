# DMA-DTC
[This repository goes with the DMA-DTC video for the UNO-R4.](https://youtu.be/GLSrd_NXoj0)


For all examples except FspDma_SoftStart_Example.ino the transfer is triggered by a falling interrupt on IRQ0.  
This is pin 2 for an Arduino R-4 Minima and pin 3 for an R-4 WiFi.
The pin is configured with a pullup so the button should be between the pin and ground.
A small capacitor is needed in order to filter any switch bounce.  

![Circuit](https://github.com/Delta-G-Videos/DMA-DTC/assets/6957239/415d74a5-cba8-46fe-b974-b4a995237dba)
