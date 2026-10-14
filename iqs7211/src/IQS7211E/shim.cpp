#include "IQS7211E/IQS7211E.h"
#include "SoftwareWire.h"

extern "C" {
#include "lib/pinout.h"
}

IQS7211E	 iqs7211e;
SoftwareWire Wire = SoftwareWire(D2, D3);

iqs7211e_s *iqs7211e_get_state(IQS7211E *self) { return &self->iqs7211e_state; }

/* Public Variables */
IQS7211E_MEMORY_MAP *iqs7211e_IQSMemoryMap(IQS7211E *self) { return &self->IQSMemoryMap; }
bool *iqs7211e_new_data_available(IQS7211E *self) { return &self->new_data_available; }

/* iqs7211e_Public Methods */
void iqs7211e_begin(IQS7211E *self, uint8_t deviceAddressIn, pin readyPinIn) {
	return self->begin(Wire, deviceAddressIn, readyPinIn);
}
bool	 iqs7211e_init(IQS7211E *self) { return self->init(); }
void	 iqs7211e_run(IQS7211E *self) { return self->run(); }
void	 iqs7211e_queueValueUpdates(IQS7211E *self) { return self->queueValueUpdates(); }
bool	 iqs7211e_readATIactive(IQS7211E *self) { return self->readATIactive(); }
uint16_t iqs7211e_getProductNum(IQS7211E *self, bool stopOrRestart) {
	return self->getProductNum(stopOrRestart);
}
uint8_t iqs7211e_getmajorVersion(IQS7211E *self, bool stopOrRestart) {
	return self->getmajorVersion(stopOrRestart);
}
uint8_t iqs7211e_getminorVersion(IQS7211E *self, bool stopOrRestart) {
	return self->getminorVersion(stopOrRestart);
}
void iqs7211e_acknowledgeReset(IQS7211E *self, bool stopOrRestart) {
	return self->acknowledgeReset(stopOrRestart);
}
void iqs7211e_ReATI(IQS7211E *self, bool stopOrRestart) { return self->ReATI(stopOrRestart); }

void iqs7211e_SW_Reset(IQS7211E *self, bool stopOrRestart) { return self->SW_Reset(stopOrRestart); }
void iqs7211e_writeMM(IQS7211E *self, bool stopOrRestart) { return self->writeMM(stopOrRestart); }
void iqs7211e_clearRDY(IQS7211E *self) { return self->clearRDY(); }
bool iqs7211e_getRDYStatus(IQS7211E *self) { return self->getRDYStatus(); }

void iqs7211e_setStreamMode(IQS7211E *self, bool stopOrRestart) {
	return self->setStreamMode(stopOrRestart);
}
void iqs7211e_setEventMode(IQS7211E *self, bool stopOrRestart) {
	return self->setEventMode(stopOrRestart);
}

void iqs7211e_updateInfoFlags(IQS7211E *self, bool stopOrRestart) {
	return self->updateInfoFlags(stopOrRestart);
}
iqs7211e_power_modes iqs7211e_getPowerMode(IQS7211E *self) { return self->getPowerMode(); }
bool				 iqs7211e_checkReset(IQS7211E *self) { return self->checkReset(); }
void iqs7211e_updateAbsCoordinates(IQS7211E *self, bool stopOrRestart, uint8_t fingerNum) {
	return self->updateAbsCoordinates(stopOrRestart, fingerNum);
}
uint16_t iqs7211e_getAbsYCoordinate(IQS7211E *self, uint8_t fingerNum) {
	return self->getAbsYCoordinate(fingerNum);
}
uint16_t iqs7211e_getAbsXCoordinate(IQS7211E *self, uint8_t fingerNum) {
	return self->getAbsXCoordinate(fingerNum);
}
bool iqs7211e_touchpad_event_occurred(IQS7211E *self) { return self->touchpad_event_occurred(); }
iqs7211e_gestures_e iqs7211e_get_touchpad_event(IQS7211E *self) {
	return self->get_touchpad_event();
}
uint8_t iqs7211e_getNumFingers(IQS7211E *self) { return self->getNumFingers(); }

void iqs7211e_force_I2C_communication(IQS7211E *self) { return self->force_I2C_communication(); }
