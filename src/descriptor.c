/**************************************************************************//**
 * @file
 * @brief   Hardware initialization functions.
 * @author  Silicon Laboratories
 * @version 1.0.0 (DM: July 14, 2014)
 *
 *******************************************************************************
 * @section License
 * (C) Copyright 2014 Silicon Labs Inc,
 * http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
 *******************************************************************************
 *
 * Descriptor information to pass to USB_Init()
 * 
 *****************************************************************************/

#include "descriptor.h"

/*** [BEGIN] USB Descriptor Information [BEGIN] ***/

#define STRING1_LEN sizeof ("M. Betz") * 2
const uint8_t code USB_MfrStr[] = // Manufacturer String
{
   STRING1_LEN, 0x03,
   'M', 0,
   '.', 0,
   ' ', 0,
   'B', 0,
   'e', 0,
   't', 0,
   'z', 0,
};

#define STRING2_LEN sizeof("Incredible Fantastic Magic Clock Box 3000") * 2
const uint8_t code USB_ProductStr[] = // Product Desc. String
{
   STRING2_LEN, 0x03,
   'I', 0,
   'n', 0,
   'c', 0,
   'r', 0,
   'e', 0,
   'd', 0,
   'i', 0,
   'b', 0,
   'l', 0,
   'e', 0,
   ' ', 0,
   'F', 0,
   'a', 0,
   'n', 0,
   't', 0,
   'a', 0,
   's', 0,
   't', 0,
   'i', 0,
   'c', 0,
   ' ', 0,
   'M', 0,
   'a', 0,
   'g', 0,
   'i', 0,
   'c', 0,
   ' ', 0,
   'C', 0,
   'l', 0,
   'o', 0,
   'c', 0,
   'k', 0,
   ' ', 0,
   'B', 0,
   'o', 0,
   'x', 0,
   ' ', 0,
   '3', 0,
   '0', 0,
   '0', 0,
   '0', 0
};

#define STRING3_LEN sizeof("1987") * 2
const uint8_t code USB_SerialStr[] = // Serial Number String
{
   STRING3_LEN, 0x03,
   '1', 0,
   '9', 0,
   '8', 0,
   '7', 0
};


const VCPXpress_Init_TypeDef InitStruct =
{
   0x10C4,                 // Vendor ID
   0xEA60,                 // Product ID
   USB_MfrStr,             // Pointer to Manufacturer String
   USB_ProductStr,         // Pointer to Product String
   USB_SerialStr,          // Pointer to Serial String
   32,                     // Max Power / 2
   0x80,                   // Power Attribute
   0x0100,                 // Device Release # (BCD format)
   false                   // Use USB FIFO space true
};
