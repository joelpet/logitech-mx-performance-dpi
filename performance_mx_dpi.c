/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Hofbauer <alex@derhofbauer.at>
 *
 * Parts of this file were heavily inspired by the work of Julien Danjou
 * (http://julien.danjou.info).
 */

#include <linux/hid.h>

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define W_INDEX 	2
#define W_VALUE 	0x0210
#define W_LENGTH 	7

#define CTRL_LED 	"\x10\x01\x80\x51\x00\x00\x00"
#define STATIC 		2
#define FLASHING	3
#define UPPER 		4
#define MIDDLE 		0
#define LOWER 		12
#define RED 		8
#define ALL_OFF 	1 << 0 | 1 << 4 | 1 << 8 | 1 << 12

#define CTRL_SENS 	"\x10\x01\x80\x63\x00\x00\x00"
#define SENS_MIN 	'\x80'


// STATIC or FLASHING
#define LED_MODE 	FLASHING


void set_matrix(unsigned char* matrix, const char* ctrl, uint16_t bits) {
    for (int i = 0; i < W_LENGTH; i++) {
        matrix[i] = ctrl[i];
    }

    matrix[4] = (bits >> 8);
    matrix[5] = bits;
}

int set_sensitivity(libusb_device_handle *handle, uint16_t sens) {
    unsigned char matrix[7];
    set_matrix(matrix, CTRL_SENS, 0);
    matrix[4] = sens;

    return libusb_control_transfer(handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
            HID_REQ_SET_REPORT, W_VALUE, W_INDEX, matrix,
            sizeof(matrix), 0);
}

int set_leds(libusb_device_handle *handle, uint16_t leds) {
    unsigned char matrix[7];
    set_matrix(matrix, CTRL_LED, leds);

    return libusb_control_transfer(handle,
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
            HID_REQ_SET_REPORT, W_VALUE, W_INDEX, matrix,
            sizeof(matrix), 0);
}


void set_sensitivity_value(libusb_device_handle *handle, uint8_t sensitivity) {
    set_sensitivity(handle, SENS_MIN | sensitivity);
    set_leds(handle, ALL_OFF);

    switch ((sensitivity - 1) / 3) {
        case 0:
            set_leds(handle, (LED_MODE << LOWER));
            break;
        case 1:
            set_leds(handle, (LED_MODE << MIDDLE));
            break;
        case 2:
            set_leds(handle, (LED_MODE << UPPER));
            break;
        case 3:
            set_leds(handle, (LED_MODE << LOWER) | (LED_MODE << MIDDLE));
            break;
        default:
            set_leds(handle, (LED_MODE << LOWER) | (LED_MODE << MIDDLE)
                    | (LED_MODE << UPPER));
            break;
    }
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Please specify a sensitivity value between 1 and 16\n");
        return EXIT_FAILURE;
    }

    int sensitivity = atoi(argv[1]);
    if (sensitivity < 1 || sensitivity > 16) {
        fprintf(stderr, "Possible sensitivity value from 1 to 16\n");
        return EXIT_FAILURE;
    }

    libusb_context *ctx;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);

    libusb_device_handle *device_handle = libusb_open_device_with_vid_pid(
            ctx, 0x046d, 0xc52b);

    if (device_handle == NULL) {
        return EXIT_FAILURE;
    }

    libusb_device *device = libusb_get_device(device_handle);

    struct libusb_device_descriptor desc;
    const struct libusb_interface_descriptor *iface_desc = NULL;

    libusb_get_device_descriptor(device, &desc);

    for (uint8_t i = 0; i < desc.bNumConfigurations; i++) {
        struct libusb_config_descriptor *config;

        libusb_get_config_descriptor(device, i, &config);

        const struct libusb_interface *iface = &config->interface[W_INDEX];

        for (int j = 0; j < iface->num_altsetting; j++) {
            iface_desc = &iface->altsetting[j];

            if (iface_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
                break;
            } else {
                iface_desc = NULL;
            }
        }

        if (iface_desc != NULL) {
            break;
        }
    }

    if (iface_desc == NULL) {
        fprintf(stdout, "Could not find valid descriptor\n");
        return EXIT_FAILURE;
    }

    libusb_detach_kernel_driver(device_handle, W_INDEX);
    libusb_claim_interface(device_handle, W_INDEX);

    fprintf(stdout, "Setting sensitivity to %d\n", sensitivity);
    set_sensitivity_value(device_handle, sensitivity);

    libusb_release_interface(device_handle, W_INDEX);
    libusb_attach_kernel_driver(device_handle, W_INDEX);

    libusb_close(device_handle);
    libusb_exit(ctx);

    return EXIT_SUCCESS;
}
