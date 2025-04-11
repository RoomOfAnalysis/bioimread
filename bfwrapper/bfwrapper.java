import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
// import java.io.File;
import java.io.IOException;
// import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
// import java.util.Hashtable;
// import java.nio.MappedByteBuffer;
// import java.nio.channels.FileChannel;
// import java.nio.channels.FileChannel.MapMode;

import loci.common.DataTools;
import loci.common.DebugTools;
import loci.common.services.ServiceFactory;
import loci.common.xml.XMLTools;
import loci.formats.FormatTools;
import loci.formats.IFormatReader;
import loci.formats.ImageReader;
import loci.formats.meta.IMetadata;
import loci.formats.meta.MetadataRetrieve;
import loci.formats.ome.OMEXMLMetadata;
import loci.formats.services.OMEXMLService;
import loci.formats.FormatException;
import ome.units.UNITS;
import ome.units.quantity.Length;
import ome.units.quantity.Time;

public class bfwrapper implements Closeable {
    private IFormatReader reader;
    private OMEXMLService service;
    private IMetadata meta;
    // private MappedByteBuffer mapped_buffer;

    public bfwrapper() {
        try {
            DebugTools.setRootLevel("ERROR");
            reader = new ImageReader();
            reader.setMetadataFiltered(true);
            reader.setOriginalMetadataPopulated(true);
            reader.setGroupFiles(false);
            ServiceFactory factory = new ServiceFactory();
            service = factory.getInstance(OMEXMLService.class);
            meta = service.createOMEXMLMetadata();
            reader.setMetadataStore(meta);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void close() throws IOException {
        reader.close();
    }

    public void close(boolean fileOnly) {
        try {
            reader.close(fileOnly);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public boolean reopenFile() {
        try {
            reader.reopenFile();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void setDebugLogLevel(String level) {
        DebugTools.setRootLevel(level);
    }

    // https://bio-formats.readthedocs.io/en/v8.0.0/developers/wsi.html
    public void setFlattenedResolutions(boolean flag) {
        reader.setFlattenedResolutions(flag);
    }

    public boolean setId(String filePath) {
        if (!reader.isThisType(filePath))
            return false;
        try {
            reader.setId(filePath);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    // https://bio-formats.readthedocs.io/en/v8.0.0/developers/file-reader.html
    // https://github.com/ome/bioformats/blob/develop/components/bio-formats-tools/src/loci/formats/tools/ImageInfo.java#L522
    public String getMetadata() {
        try {
            String info = Thread.currentThread().getName() +
                    ": path=" + reader.getCurrentFile() +
                    ", seriesCount=" + reader.getSeriesCount() +
                    ", imageCount=" + reader.getImageCount() +
                    ", sizeX=" + reader.getSizeX() +
                    ", sizeY=" + reader.getSizeY() +
                    ", sizeZ=" + reader.getSizeZ() +
                    ", sizeC=" + reader.getSizeC() +
                    ", sizeT=" + reader.getSizeT() +
                    ", imageName=" + meta.getImageName(0); // https://javadoc.scijava.org/Bio-Formats/loci/formats/meta/MetadataRetrieve.html
            return info;
        } catch (Exception e) {
            e.printStackTrace();
            return "Error: " + e.getMessage();
        }
    }

    public String getOMEXML() {
        try {
            // FIXME: seems have to use new reader to obtain XML, since XML always stays the
            // same after `setID` to different file

            // String xml = service.getOMEXML((MetadataRetrieve) meta);
            // XMLTools.indentXML(xml, 3, true);
            // return xml;
            return ((OMEXMLMetadata) meta).dumpXML();

            // the following will change after `setID` to different file

            // System.out.println(service.getOriginalMetadata((OMEXMLMetadata) meta));
            // return service.getOriginalMetadata((OMEXMLMetadata) meta).toString();
            // return reader.getSeriesMetadata().toString();
            // return reader.getGlobalMetadata().toString();
        } catch (Exception e) {
            e.printStackTrace();
            return "Error: " + e.getMessage();
        }
    }

    public int getImageCount() {
        return reader.getImageCount();
    }

    public int getSeriesCount() {
        return reader.getSeriesCount();
    }

    public void setSeries(int no) {
        reader.setSeries(no);
    }

    public int getSeries() {
        return reader.getSeries();
    }

    /**
     * Checks if the image planes in the file have more than one channel per
     * {@link #openBytes} call.
     * This method returns true if and only if {@link #getRGBChannelCount()}
     * returns a value greater than 1.
     */
    public boolean isRGB() {
        return reader.isRGB();
    }

    /** Gets the size of the X dimension. */
    public int getSizeX() {
        return reader.getSizeX();
    }

    /** Gets the size of the Y dimension. */
    public int getSizeY() {
        return reader.getSizeY();
    }

    /** Gets the size of the Z dimension. */
    public int getSizeZ() {
        return reader.getSizeZ();
    }

    /** Gets the size of the C dimension. */
    public int getSizeC() {
        return reader.getSizeC();
    }

    /** Gets the size of the T dimension. */
    public int getSizeT() {
        return reader.getSizeT();
    }

    /**
     * Gets the effective size of the C dimension, guaranteeing that
     * getEffectiveSizeC() * getSizeZ() * getSizeT() == getImageCount()
     * regardless of the result of isRGB().
     */
    public int getEffectiveSizeC() {
        return reader.getEffectiveSizeC();
    }

    // https://github.com/ome/ome-model/blob/master/ome-xml/src/main/java/ome/units/UNITS.java
    // mm
    public double getPhysSizeX() {
        return mm(meta.getPixelsPhysicalSizeX(0), 1.0);
    }

    // mm
    public double getPhysSizeY() {
        return mm(meta.getPixelsPhysicalSizeY(0), 1.0);
    }

    // mm
    public double getPhysSizeZ() {
        return mm(meta.getPixelsPhysicalSizeZ(0), 1.0);
    }

    // s
    public double getPhysSizeT() {
        return sec(meta.getPixelsTimeIncrement(0), 1.0);
    }

    // https://github.com/ome/bioformats/blob/develop/components/formats-api/src/loci/formats/FormatTools.java#L99
    /**
     * Gets the pixel type.
     * 
     * @return the pixel type as an enumeration from {@link FormatTools}
     *         <i>static</i> pixel types such as {@link FormatTools#INT8}.
     */
    public int getPixelType() {
        return reader.getPixelType();
    }

    /**
     * Gets the number of valid bits per pixel. The number of valid bits per
     * pixel is always less than or equal to the number of bits per pixel
     * that correspond to {@link #getPixelType()}.
     */
    public int getBitsPerPixel() {
        return reader.getBitsPerPixel();
    }

    /**
     * Gets the number of channels returned with each call to openBytes.
     * The most common case where this value is greater than 1 is for interleaved
     * RGB data, such as a 24-bit color image plane. However, it is possible for
     * this value to be greater than 1 for non-interleaved data, such as an RGB
     * TIFF with Planar rather than Chunky configuration.
     */
    public int getRGBChannelCount() {
        return reader.getRGBChannelCount();
    }

    /** Gets whether the data is in little-endian format. */
    public boolean isLittleEndian() {
        return reader.isLittleEndian();
    }

    /**
     * Gets a five-character string representing the
     * dimension order in which planes will be returned. Valid orders are:
     * <ul>
     * <li>XYCTZ</li>
     * <li>XYCZT</li>
     * <li>XYTCZ</li>
     * <li>XYTZC</li>
     * <li>XYZCT</li>
     * <li>XYZTC</li>
     * </ul>
     * In cases where the channels are interleaved (e.g., CXYTZ), C will be
     * the first dimension after X and Y (e.g., XYCTZ) and the
     * {@link #isInterleaved()} method will return true.
     */
    public String getDimensionOrder() {
        return reader.getDimensionOrder();
    }

    /**
     * Gets whether the dimension order and sizes are known, or merely guesses.
     */
    public boolean isOrderCertain() {
        return reader.isOrderCertain();
    }

    /**
     * Retrieves how many bytes per pixel the current plane or section has.
     * 
     * @param pixelType the pixel type as retrieved from
     *                  {@link IFormatReader#getPixelType()}.
     * @return the number of bytes per pixel.
     * @see IFormatReader#getPixelType()
     */
    public int getBytesPerPixel() {
        return FormatTools.getBytesPerPixel(reader.getPixelType());
    }

    /** Returns the size in bytes of a single plane. */
    public int getPlaneSize() {
        // r.getSizeX() * r.getSizeY() * r.getRGBChannelCount() *
        // getBytesPerPixel(r.getPixelType())
        return FormatTools.getPlaneSize(reader);
    }

    // javap.exe -s -public .\bioformats_package\ome\xml\meta\MetadataRetrieve.class
    public int[] getChannelColor(int channel) {
        var color = meta.getChannelColor(getSeries(), channel);
        return new int[] { color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha() };
    }

    // https://github.com/ome/bioformats/blob/develop/components/formats-api/src/loci/formats/IFormatReader.java#L245
    /**
     * Obtains the specified image plane from the current file as a byte array.
     */
    public byte[] openBytes(int no) {
        try {
            return reader.openBytes(no);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Obtains the specified image plane from the current file into a
     * pre-allocated byte array of
     * (sizeX * sizeY * bytesPerPixel * RGB channel count).
     */
    public byte[] openBytes(int no, byte[] buf) {
        try {
            return reader.openBytes(no, buf);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Obtains a sub-image of the specified image plane,
     * whose upper-left corner is given by (x, y).
     */
    public byte[] openBytes(int no, int x, int y, int w, int h) {
        try {
            return reader.openBytes(no, x, y, w, h);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Obtains a sub-image of the specified image plane
     * into a pre-allocated byte array.
     *
     * @param no  the plane index within the current series.
     * @param buf a pre-allocated buffer.
     * @param x   X coordinate of the upper-left corner of the sub-image
     * @param y   Y coordinate of the upper-left corner of the sub-image
     * @param w   width of the sub-image
     * @param h   height of the sub-image
     * @return the pre-allocated buffer <code>buf</code> for convenience.
     */
    public byte[] openBytes(int no, byte[] buf, int x, int y, int w, int h) {
        try {
            return reader.openBytes(no, buf, x, y, w, h);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /** Returns the optimal sub-image width for use with openBytes. */
    public int getOptimalTileWidth() {
        return reader.getOptimalTileWidth();
    }

    /** Returns the optimal sub-image height for use with openBytes. */
    public int getOptimalTileHeight() {
        return reader.getOptimalTileHeight();
    }

    /** Get the size of the X dimension for the thumbnail. */
    public int getThumbSizeX() {
        return reader.getThumbSizeX();
    }

    /** Get the size of the Y dimension for the thumbnail. */
    public int getThumbSizeY() {
        return reader.getThumbSizeY();
    }

    /**
     * Obtains a thumbnail for the specified image plane from the current file,
     * as a byte array.
     */
    public byte[] openThumbBytes(int no) {
        try {
            return reader.openThumbBytes(no);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    // ensure bytes in little endian
    // https://github.com/scifio/scifio-itk-bridge/blob/master/src/main/java/io/scif/itk/SCIFIOITKBridge.java#L402
    public byte[] openPlane(int no) {
        final int bpp = FormatTools.getBytesPerPixel(reader.getPixelType());
        final int rgbChannelCount = reader.getRGBChannelCount();
        if (rgbChannelCount == 1)
            return getBytes(DataTools.makeDataArray(openBytes(no), bpp, FormatTools
                    .isFloatingPoint(reader.getPixelType()),
                    reader
                            .isLittleEndian()));
        final byte[] pixel = new byte[bpp];
        final boolean isInterleaved = reader.isInterleaved();
        final int xLen = reader.getSizeX();
        final int yLen = reader.getSizeY();
        byte[] image = openBytes(no);
        final ByteArrayOutputStream out = new ByteArrayOutputStream(bpp * xLen * yLen * rgbChannelCount);
        for (int y = 0; y < yLen; y++) {
            for (int x = 0; x < xLen; x++) {
                for (int i = 0; i < rgbChannelCount; i++) {
                    for (int b = 0; b < bpp; b++) {
                        int index = 0;
                        if (isInterleaved) {
                            index = ((y * xLen + x) * rgbChannelCount + i) * bpp + b;
                        } else {
                            index = ((i * yLen + y) * xLen + x) * bpp + b;
                        }
                        pixel[b] = image[index];
                    }
                    try {
                        out.write(getBytes(DataTools.makeDataArray(pixel, bpp, FormatTools
                                .isFloatingPoint(reader.getPixelType()),
                                reader
                                        .isLittleEndian())));
                        out.flush();
                    } catch (IOException e) {
                        e.printStackTrace();
                        return null;
                    }
                }
            }
        }
        return out.toByteArray();
    }

    public void openPlane(int no, ByteBuffer buffer) {
        buffer.clear();
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.put(openPlane(no));
    }

    public boolean openPlanes(int start, int length, ByteBuffer buffer) {
        try {
            buffer.clear();
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            for (int i = 0; i < length; i++)
                buffer.put(openPlane(i + start));
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    // public boolean openPlanes(int start, int length) {
    // try {
    // if (mapped_buffer == null) {
    // mapped_buffer = new RandomAccessFile(File.createTempFile("tmp", null),
    // "rw").getChannel()
    // .map(MapMode.READ_WRITE, 0, Integer.MAX_VALUE);
    // mapped_buffer.order(ByteOrder.LITTLE_ENDIAN);
    // }
    // mapped_buffer.clear();
    // for (int i = 0; i < length; i++)
    // mapped_buffer.put(openPlane(i + start));
    // return true;
    // } catch (Exception e) {
    // e.printStackTrace();
    // return false;
    // }
    // }

    /**
     * Gets the rasterized index corresponding
     * to the given Z, C and T coordinates (real sizes).
     */
    public int getPlaneIndex(int z, int c, int t) {
        return reader.getIndex(z, c, t);
    }

    /**
     * Gets the Z, C and T coordinates (real sizes) corresponding to the
     * given rasterized index value.
     */
    public int[] getZCTCoords(int index) {
        return reader.getZCTCoords(index);
    }

    /**
     * Gets the 8-bit color lookup table associated with
     * the most recently opened image.
     * If no image planes have been opened, or if {@link #isIndexed()} returns
     * false, then this may return null. Also, if {@link #getPixelType()} returns
     * anything other than {@link FormatTools#INT8} or {@link FormatTools#UINT8},
     * this method will return null.
     */
    public byte[][] get8BitLookupTable() {
        try {
            return reader.get8BitLookupTable();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Gets the 16-bit color lookup table associated with
     * the most recently opened image.
     * If no image planes have been opened, or if {@link #isIndexed()} returns
     * false, then this may return null. Also, if {@link #getPixelType()} returns
     * anything other than {@link FormatTools#INT16} or {@link
     * FormatTools#UINT16}, this method will return null.
     */
    public short[][] get16BitLookupTable() {
        try {
            return reader.get16BitLookupTable();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    // only available after `setFlattenedResolutions`
    public int getResolutionCount() {
        return reader.getResolutionCount();
    }

    // only available after `setFlattenedResolutions`
    public void setResolution(int level) {
        reader.setResolution(level);
    }

    private byte[] getBytes(final Object data) {
        if (data instanceof byte[]) {
            return (byte[]) data;
        } else if (data instanceof short[]) {
            return DataTools.shortsToBytes((short[]) data, true);
        } else if (data instanceof int[]) {
            return DataTools.intsToBytes((int[]) data, true);
        } else if (data instanceof long[]) {
            return DataTools.longsToBytes((long[]) data, true);
        } else if (data instanceof double[]) {
            return DataTools.doublesToBytes((double[]) data, true);
        } else if (data instanceof float[]) {
            return DataTools.floatsToBytes((float[]) data, true);
        }
        return null;
    }

    private double mm(final Length l, final double defaultValue) {
        return l != null && l.unit().isConvertible(UNITS.MILLIMETER) ? l.value(UNITS.MILLIMETER).doubleValue()
                : defaultValue;
    }

    private double sec(final Time t, final double defaultValue) {
        return t != null && t.unit().isConvertible(UNITS.SECOND) ? t.value(UNITS.SECOND).doubleValue() : defaultValue;
    }
}