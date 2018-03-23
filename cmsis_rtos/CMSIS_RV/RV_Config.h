/*-----------------------------------------------------------------------------
 *      Name:         RV_Config.h 
 *      Purpose:      RV Config header
 *----------------------------------------------------------------------------
 *      Copyright(c) KEIL - An ARM Company
 *----------------------------------------------------------------------------*/
#ifndef __RV_CONFIG_H
#define __RV_CONFIG_H

#include "ARMCM3.h"

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h> Common Test Settings
// <o> Print Output Format <0=> Plain Text <1=> XML
// <i> Set the test results output format to plain text or XML
#ifndef PRINT_XML_REPORT
#define PRINT_XML_REPORT            0 
#endif
// <o> Buffer size for assertions results
// <i> Set the buffer size for assertions results buffer
#define BUFFER_ASSERTIONS           128  
// </h>
  
#endif /* __RV_CONFIG_H */
