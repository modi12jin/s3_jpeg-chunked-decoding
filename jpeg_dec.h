#ifndef _JPEG_DEC_H_
#define _JPEG_DEC_H_

#include <ESP32_JPEG_Library.h>
#include "FS.h"

size_t getFileSize(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  if (!file) {
    Serial.println("- failed to open file for getting size");
    return 0;
  }

  size_t len = file.size();
  file.close();
  return len;
}

void readFile(fs::FS &fs, const char *path, uint8_t *buf, size_t buf_len) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  size_t len = file.size();
  len = len > buf_len ? buf_len : len;
  while (len) {
    size_t read_len = len > 512 ? 512 : len;
    read_len = file.read(buf, read_len);
    buf += read_len;
    len -= read_len;
  }

  file.close();

  Serial.println("Read file done");
}

jpeg_error_t esp_jpeg_decoder_one_picture_block_out(unsigned char *in_buf, int in_len,int (*jpegDrawCallback)(jpeg_dec_io_t *jpeg_io,jpeg_dec_header_info_t *out_info)) {
  unsigned char *output_block = NULL;
  int output_len = 0;
  jpeg_error_t ret = JPEG_ERR_OK;
  jpeg_dec_io_t *jpeg_io = NULL;
  jpeg_dec_header_info_t *out_info = NULL;
  jpeg_dec_handle_t *jpeg_dec = NULL;

  // Generate configuration
  jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
  config.block_enable = 1;

  // Create jpeg_dec
  jpeg_dec = jpeg_dec_open(&config);
  if (jpeg_dec == NULL) {
    ret = JPEG_ERR_PAR;
    Serial.println("JPEG open error");
    goto jpeg_dec_failed;
  }

  // Create io_callback handle
  jpeg_io = (jpeg_dec_io_t *)calloc(1, sizeof(jpeg_dec_io_t));
  if (jpeg_io == NULL) {
    ret = JPEG_ERR_MEM;
    goto jpeg_dec_failed;
  }

  // Create out_info handle
  out_info = (jpeg_dec_header_info_t *)calloc(1, sizeof(jpeg_dec_header_info_t));
  if (out_info == NULL) {
    ret = JPEG_ERR_MEM;
    goto jpeg_dec_failed;
  }

  // Set input buffer and buffer len to io_callback
  jpeg_io->inbuf = in_buf;
  jpeg_io->inbuf_len = in_len;

  // Parse jpeg picture header and get picture for user and decoder
  ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
  if (ret != JPEG_ERR_OK) {
    Serial.printf("JPEG parse header error, ret = %d\n", ret);
    goto jpeg_dec_failed;
  }

  output_len = out_info->width * (out_info->y_factory[0] << 3) * 2;
  // Malloc output block buffer
  output_block = (unsigned char *)jpeg_malloc_align(output_len, 16);
  if (output_block == NULL) {
    ret = JPEG_ERR_MEM;
    Serial.printf("Output_block malloc fail, line: %d\n", __LINE__);
    return ret;
  }
  jpeg_io->outbuf = output_block;

  // Save to sdcard
  // FILE *f_out = fopen("/sdcard/dec_out.bin", "wb");
  // if (f_out == NULL) {
  //     ESP_LOGE(TAG, "File open fail");
  //     ret = JPEG_ERR_FAIL;
  //     goto jpeg_dec_failed;
  // }

  // Decode jpeg data
  // Each time output (jpeg_io->cur_line) line, (jpeg_io->cur_len) bytes
  while (jpeg_io->output_line < jpeg_io->output_height) {
    ret = jpeg_dec_process(jpeg_dec, jpeg_io);
    if (ret != JPEG_ERR_OK) {
      Serial.printf("JPEG process error, ret = %d, line: %d\n", ret, __LINE__);
      goto jpeg_dec_failed;
    } 

    jpegDrawCallback(jpeg_io,out_info);

    // Print output info
    //Serial.printf("output line/height: %d/%d, cur_height: %d, cur_len: %d, addr: %p\n", jpeg_io->output_line, jpeg_io->output_height, jpeg_io->cur_line, jpeg_io->cur_len, jpeg_io->outbuf);

    // Use output data
    // Save to sdcard
    // fwrite(jpeg_io->outbuf, 1, jpeg_io->cur_len, f_out);
  }
  // Save to sdcard
  // fclose(f_out);

  // Decoder deinitialize
jpeg_dec_failed:
  jpeg_dec_close(jpeg_dec);
  if (jpeg_io) {
    free(jpeg_io);
  }
  if (out_info) {
    free(out_info);
  }
  jpeg_free_align(output_block);
  return ret;
}

#endif // _JPEG_DEC_H_