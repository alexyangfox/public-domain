man_MANS=MAGclose.3 MAGcomment.3 MAGcommentSize.3 MAGimageWidth.3 MAGmacCode.3 \
	MAGmacFlags.3 MAGmacType.3 MAGopen.3 MAGpixelWidth.3 MAGscrMode200Lines.3 \
	MAGscrModeCode.3 MAGscrModeIsDigital.3 MAGscrModeNumColors.3 MAGuserName.3 \
	MAGversion.3 MAGimageHeight.3 MAGimageStartX.3 MAGimageStartY.3 \
	MAGimageEndX.3 MAGimageEndY.3 MAGpalette.3 MAGReader.3 MAGReaderDelete.3 \
	MAGReaderNextPixel.3 \
	libmaglo.7 magtoppm.1
EXTRA_DIST=$(man_MANS) mkhtml.sh

MAGimageHeight.3: MAGimageWidth.3
	cp MAGimageWidth.3 MAGimageHeight.3

MAGimageStartX.3: MAGimageWidth.3
	cp MAGimageWidth.3 MAGimageStartX.3

MAGimageStartY.3: MAGimageWidth.3
	cp MAGimageWidth.3 MAGimageStartY.3

MAGimageEndX.3: MAGimageWidth.3
	cp MAGimageWidth.3 MAGimageEndX.3

MAGimageEndY.3: MAGimageWidth.3
	cp MAGimageWidth.3 MAGimageEndY.3

clean:
	rm MAGimageHeight.3 MAGimageStartX.3 MAGimageEndX.3 MAGimageStartY.3 MAGimageEndY.3
