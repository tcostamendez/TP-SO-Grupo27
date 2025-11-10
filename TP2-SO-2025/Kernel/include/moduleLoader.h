#ifndef MODULELOADER_H
#define MODULELOADER_H

/**
 * @brief Load kernel modules from a packed payload into target addresses.
 * @param payloadStart Start of packed modules payload.
 * @param moduleTargetAddress Array of target addresses to place modules.
 */
void loadModules(void * payloadStart, void ** moduleTargetAddress);

#endif