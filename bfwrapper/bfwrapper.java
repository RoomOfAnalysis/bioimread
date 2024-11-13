import java.io.Closeable;
import java.io.IOException;
import java.util.Arrays;
import loci.common.DataTools;
import loci.common.services.ServiceFactory;
import loci.common.xml.XMLTools;
import loci.formats.FormatTools;
import loci.formats.ImageReader;
import loci.formats.meta.IMetadata;
import loci.formats.meta.MetadataRetrieve;
import loci.formats.services.OMEXMLService;
import loci.formats.FormatException;

public class bfwrapper implements Closeable {
    private ImageReader reader;
    private OMEXMLService service;
    private IMetadata meta;

    public bfwrapper() {
        try {
            reader = new ImageReader();
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

    public boolean setId(String filePath) {
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
            String xml = service.getOMEXML((MetadataRetrieve) meta);
            XMLTools.indentXML(xml, 3, true);
            return xml;
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
     * Gets the effective size of the C dimension, guaranteeing that
     * getEffectiveSizeC() * getSizeZ() * getSizeT() == getImageCount()
     * regardless of the result of isRGB().
     */
    public int getEffectiveSizeC() {
        return reader.getEffectiveSizeC();
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
    public String getChannelColor(int channel) {
        var color = meta.getChannelColor(getSeries(), channel);
        int[] rgba = { color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha() };
        return Arrays.toString(rgba);
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
}