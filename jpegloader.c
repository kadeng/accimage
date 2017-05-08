#include "accimage.h"

#include <setjmp.h>

#include <jpeglib.h>
#ifndef LIBJPEG_TURBO_VERSION
    #error libjpeg-turbo required (not IJG libjpeg)
#endif


struct accimage_jpeg_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};


static void accimage_jpeg_error_exit(j_common_ptr cinfo) {
    struct accimage_jpeg_error_mgr* accimage_err = (struct accimage_jpeg_error_mgr*) cinfo->err;
    longjmp(accimage_err->setjmp_buffer, 1);
}


void image_from_bytes(ImageObject* self, const unsigned char* bytes, const unsigned long len) {
    struct jpeg_decompress_struct state = { 0 };
    struct accimage_jpeg_error_mgr jpeg_error;
    unsigned char* buffer = NULL;

    state.err = jpeg_std_error(&jpeg_error.pub);
    jpeg_error.pub.error_exit = accimage_jpeg_error_exit;
    if (setjmp(jpeg_error.setjmp_buffer)) {
        char error_message[JMSG_LENGTH_MAX];
        (*state.err->format_message)((j_common_ptr) &state, error_message);
        PyErr_Format(PyExc_IOError, "JPEG decoding failed: %s from memory buffer",
            error_message);

        goto cleanup;
    }

    jpeg_create_decompress(&state);
    jpeg_mem_src(&state, (unsigned char *)bytes, len);
    jpeg_read_header(&state, TRUE);

    state.dct_method = JDCT_ISLOW;
    state.output_components = 3;
    state.out_color_components = 3;
    state.out_color_space = JCS_RGB;

    jpeg_start_decompress(&state);

    buffer = malloc(state.output_components * state.output_width * state.output_height);
    if (buffer == NULL) {
        PyErr_NoMemory();
        goto cleanup;
    }

    while (state.output_scanline < state.output_height) {
        unsigned char* row = buffer + state.output_scanline * state.output_width * state.output_components;
        jpeg_read_scanlines(&state, &row, 1);
    }

    jpeg_finish_decompress(&state);

    /* Success. Commit object state */
    self->buffer = buffer;
    buffer = NULL;

    self->channels = 3;
    self->height = state.output_height;
    self->width = state.output_width;
    self->row_stride = state.output_width;
    self->y_offset = 0;
    self->x_offset = 0;

cleanup:
    jpeg_destroy_decompress(&state);

    free(buffer);

}

void image_from_jpeg(ImageObject* self, const char* path) {
    struct jpeg_decompress_struct state = { 0 };
    struct accimage_jpeg_error_mgr jpeg_error;
    FILE* file = NULL;
    unsigned char* buffer = NULL;

    file = fopen(path, "rb");
    if (file == NULL) {
        PyErr_Format(PyExc_IOError, "failed to open file %s", path);
        goto cleanup;
    }

    state.err = jpeg_std_error(&jpeg_error.pub);
    jpeg_error.pub.error_exit = accimage_jpeg_error_exit;
    if (setjmp(jpeg_error.setjmp_buffer)) {
        char error_message[JMSG_LENGTH_MAX];
        (*state.err->format_message)((j_common_ptr) &state, error_message);
        PyErr_Format(PyExc_IOError, "JPEG decoding failed: %s in file %s",
            error_message, path);

        goto cleanup;
    }

    jpeg_create_decompress(&state);
    jpeg_stdio_src(&state, file);
    jpeg_read_header(&state, TRUE);

    state.dct_method = JDCT_ISLOW;
    state.output_components = 3;
    state.out_color_components = 3;
    state.out_color_space = JCS_RGB;

    jpeg_start_decompress(&state);

    buffer = malloc(state.output_components * state.output_width * state.output_height);
    if (buffer == NULL) {
        PyErr_NoMemory();
        goto cleanup;
    }

    while (state.output_scanline < state.output_height) {
        unsigned char* row = buffer + state.output_scanline * state.output_width * state.output_components;
        jpeg_read_scanlines(&state, &row, 1);
    }

    jpeg_finish_decompress(&state);

    /* Success. Commit object state */
    self->buffer = buffer;
    buffer = NULL;

    self->channels = 3;
    self->height = state.output_height;
    self->width = state.output_width;
    self->row_stride = state.output_width;
    self->y_offset = 0;
    self->x_offset = 0;

cleanup:
    jpeg_destroy_decompress(&state);

    free(buffer);

    if (file != NULL) {
        fclose(file);
    }
}
