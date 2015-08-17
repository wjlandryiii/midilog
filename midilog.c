/*
 * Copyright 2015 Joseph Landry All Rights Reserved
 */

#include <stdio.h>
#include <windows.h>

#pragma warning (disable:4100)

struct midi_in_device {
	int connected;
	HMIDIIN hmi;
	MIDIINCAPS caps;
	struct midi_in_device *next;
};

struct midi_in_device *midiDeviceList = NULL;

void CALLBACK MyMidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2){
	struct midi_in_device *m = (struct midi_in_device *) dwInstance;
	unsigned char status;
	unsigned char data1;
	unsigned char data2;

	if(wMsg == MIM_DATA){
		status = (unsigned char) ((dwParam1 & 0x000000FF)>>0);
		data1  = (unsigned char) ((dwParam1 & 0x0000FF00)>>8);
		data2  = (unsigned char) ((dwParam1 & 0x00FF0000)>>16);
		printf("%02x %02x %02x FROM: \"%s\"\n", status, data1, data2, m->caps.szPname);
	} else if(wMsg == MM_MIM_CLOSE){
		m->connected = 0;
	} else if(wMsg == MM_MIM_OPEN){
		;
	} else {
		printf("unknown message: %08x FROM: \"%s\"\n", wMsg, m->caps.szPname);
	}
}

void checkForMidiDeviceChanges(){
	struct midi_in_device_update {
		int has_handle;
		HMIDIIN hmi;
	};
	
	UINT i, n;
	struct midi_in_device_update updateList[64] = {0};
	struct midi_in_device *device;
	struct midi_in_device **pdevice;

	n = midiInGetNumDevs();
	if(n >= 64){
		printf("Too many MIDI devices\n");
		return;
	}

	pdevice = &midiDeviceList;
	device = midiDeviceList;
	while(device != NULL){
		MMRESULT result;
		UINT id;

		if(device->connected){
			result = midiInGetID(device->hmi, &id);
			if(result == MMSYSERR_NOERROR){
				if(id < 64){
					updateList[id].has_handle = 1;
					updateList[id].hmi = device->hmi;
				}
			}
			pdevice = &device->next;
			device = device->next;
		} else {
			struct midi_in_device *disconnectedDevice;
			disconnectedDevice = device;
			*pdevice = device->next;
			device = device->next;
			printf("REMOVED \"%s\"\n", disconnectedDevice->caps.szPname);
			free(disconnectedDevice);
		}
	}

	for(i = 0; i < n; i++){
		if(updateList[i].has_handle == 0){
			MMRESULT result;
			struct midi_in_device *newDevice;

			newDevice = calloc(1, sizeof(struct midi_in_device));
			if(newDevice != NULL){
				newDevice->connected = 1;

				result = midiInGetDevCaps(i, &newDevice->caps, sizeof(MIDIINCAPS));
				if(result == MMSYSERR_NOERROR){
					printf("Trying to connect to: %s\n", newDevice->caps.szPname);
					result = midiInOpen(&newDevice->hmi, i, (DWORD_PTR)MyMidiInProc, (DWORD_PTR)newDevice, CALLBACK_FUNCTION);
					if(result == MMSYSERR_NOERROR){
						result = midiInStart(newDevice->hmi);
						if(result == MMSYSERR_NOERROR){
							newDevice->next = midiDeviceList;
							midiDeviceList = newDevice;
							printf("Connected: \"%s\"\n", newDevice->caps.szPname);
						} else {
							free(newDevice);
							printf("midiInStart() error\n");
							continue;
						}
					} else {
						free(newDevice);
						printf("midiInOpen() error\n");
						continue;
					}
				} else {
					free(newDevice);
					printf("midiInGetDevCaps() error\n");
					continue;
				}

			} else {
				printf("MEMORY ERROR\n");
				exit(1);
			}

		}
	}
}


int main(int argc, char *argv[]){
	for(;;){
		/* Windows doesn't offer you an event to tell you when MIDI devices are plugged in. */
		/* So we have to check it every so often. */
		checkForMidiDeviceChanges();
		Sleep(1000);
	}
	// return 0;  // cl.exe warning "unreachable code"
}
