#include "SD_MMC.h"
#include "jpeg_dec.h"
#include "pins_config.h"
#include "src/lcd/nv3041a_lcd.h"
nv3041a_lcd lcd = nv3041a_lcd(TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3, TFT_QSPI_RST);

#define TEST_NUM 10
#define TEST_IMAGE_FILE_PATH "/img_480_272.jpg"
#define TEST_IMAGE_WIDTH (480)
#define TEST_IMAGE_HEIGHT (272)

//jpeg绘制回调
static int jpegDrawCallback(jpeg_dec_io_t *jpeg_io, jpeg_dec_header_info_t *out_info) {
  lcd.draw16bitbergbbitmap(0, jpeg_io->output_line - jpeg_io->cur_line, out_info->width, jpeg_io->cur_line, (uint16_t *)jpeg_io->outbuf);
  return 1;
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */
  Serial.println("Hello Arduino!");

  lcd.begin();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  pinMode(SDMMC_CS, OUTPUT);
  digitalWrite(SDMMC_CS, HIGH);
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
  if (!SD_MMC.begin("/root", true)) {
    Serial.println("SDMMC Mount Failed");
    return;
  }

  uint8_t *image_jpeg = NULL;
  size_t image_jpeg_size = getFileSize(SD_MMC, TEST_IMAGE_FILE_PATH);
  /* The buffer used by JPEG decoder must be 16-byte aligned */
  image_jpeg = (unsigned char *)jpeg_malloc_align(image_jpeg_size, 16);
  if (image_jpeg == NULL) {
    Serial.println("Image memory allocation failed");
    return;
  }
  readFile(SD_MMC, TEST_IMAGE_FILE_PATH, image_jpeg, image_jpeg_size);

  jpeg_error_t ret = JPEG_ERR_OK;
  uint32_t t = millis();
  for (int i = 0; i < TEST_NUM; i++) {
    esp_jpeg_decoder_one_picture_block_out(image_jpeg, image_jpeg_size, jpegDrawCallback);
  }
  Serial.printf("JPEG decode %d images, average time is %d ms\n", TEST_NUM, (millis() - t) / TEST_NUM);
  jpeg_free_align(image_jpeg);
}

void loop() {
}
