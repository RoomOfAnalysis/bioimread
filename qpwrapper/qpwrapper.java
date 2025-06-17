import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.IOException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

import javax.imageio.ImageIO;

import qupath.lib.common.GeneralTools;
import qupath.lib.images.servers.bioformats.BioFormatsServerBuilder;
import qupath.lib.images.servers.bioformats.BioFormatsImageServer;
import qupath.lib.images.servers.ImageServer;
import qupath.lib.images.servers.ImageServers;
import qupath.lib.images.servers.ImageServerMetadata;
import qupath.lib.images.servers.PixelCalibration;
import qupath.lib.images.servers.TileRequest;
import qupath.lib.regions.ImageRegion;

public class qpwrapper implements AutoCloseable {
    private BioFormatsServerBuilder builder;
    private ImageServer<BufferedImage> server; // https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/ImageServer.java
    private ImageServerMetadata meta;
    private PixelCalibration pixcal;

    public qpwrapper(String path) {
        try {
            builder = new BioFormatsServerBuilder();
            server = builder.buildServer(Paths.get(path).toUri());
            meta = server.getMetadata();
            pixcal = server.getPixelCalibration();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void close() throws Exception {
        server.close();
    }

    // https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/ImageServerMetadata.java#L822
    public String getMetadata() {
        try {
            return String.format(
                    "%s: %d x %d (c=%d, z=%d, t=%d), bpp=%s, mag=%.2f, downsamples=[%s], res=[%.4f,%.4f,%.4f]",
                    server.getPath(), server.getWidth(), server.getHeight(),
                    server.nChannels(), server.nZSlices(), server.nTimepoints(),
                    server.getPixelType().toString(), meta.getMagnification(),
                    GeneralTools.arrayToString(Locale.getDefault(Locale.Category.FORMAT),
                            server.getPreferredDownsamples(),
                            4),
                    pixcal.getPixelWidthMicrons(), pixcal.getPixelHeightMicrons(), pixcal.getZSpacingMicrons());
        } catch (Exception e) {
            e.printStackTrace();
        }
        return server.getMetadata().toString();
    }

    public String getPath() {
        return server.getPath();
    }

    /**
     * Get the highest magnification value, or Double.NaN if this is unavailable.
     * 
     * @return
     */
    public double getMagnification() {
        return meta.getMagnification();
    }

    /**
     * Get the preferred tile width, which can be used to optimize pixel requests
     * for large images.
     * 
     * @return
     */
    public int getPreferredTileWidth() {
        return meta.getPreferredTileWidth();
    }

    /**
     * Get the preferred tile height, which can be used to optimize pixel requests
     * for large images.
     * 
     * @return
     */
    public int getPreferredTileHeight() {
        return meta.getPreferredTileHeight();
    }

    /**
     * True if the image has 8-bit red, green &amp; blue channels (and nothing
     * else), false otherwise.
     * 
     * @return
     */
    public boolean isRGB() {
        return server.isRGB();
    }

    /** Gets the size of the X dimension. */
    public int getSizeX() {
        return server.getWidth();
    }

    /** Gets the size of the Y dimension. */
    public int getSizeY() {
        return server.getHeight();
    }

    /** Gets the size of the Z dimension. */
    public int getSizeZ() {
        return server.nZSlices();
    }

    /** Gets the size of the C dimension. */
    public int getSizeC() {
        return server.nChannels();
    }

    /** Gets the size of the T dimension. */
    public int getSizeT() {
        return server.nTimepoints();
    }

    // mm
    public double getPhysSizeX() {
        return pixcal.getPixelWidthMicrons() * 1000.0 * server.getWidth();
    }

    // mm
    public double getPhysSizeY() {
        return pixcal.getPixelHeightMicrons() * 1000.0 * server.getHeight();
    }

    // mm
    public double getPhysSizeZ() {
        return pixcal.getZSpacingMicrons() * 1000.0 * server.nZSlices();
    }

    // s
    public double getPhysSizeT() {
        return pixcal.getTimeUnit().toSeconds(1) * server.nTimepoints();
    }

    /**
     * Gets the pixel type.
     * 
     * @return the pixel type as an enumeration from
     *         https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/PixelType.java#L30
     *         <i>static</i> pixel types such as UINT8.
     */
    public int getPixelType() {
        return server.getPixelType().ordinal();
    }

    /**
     * Gets the number of valid bits per pixel
     * correspond to {@link #getPixelType()}.
     * 
     * @return the number of bits per pixel.
     * @see https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/PixelType.java#L82
     */
    public int getBitsPerPixel() {
        return meta.getPixelType().getBitsPerPixel();
    }

    /**
     * Retrieves how many bytes per pixel the current plane or section has.
     * {@link #getPixelType()}.
     * 
     * @return the number of bytes per pixel.
     * @see https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/PixelType.java#L108
     */
    public int getBytesPerPixel() {
        return meta.getPixelType().getBytesPerPixel();
    }

    /**
     * Request information for one channel (0-based index).
     * 
     * @param channel
     * @return ARGB 32bits
     * 
     * @see ImageServerMetadata#getChannels()
     */
    public int getChannel(int channel) {
        return server.getChannel(channel).getColor().intValue();
    }

    /**
     * Get a list of 'associated images', e.g. thumbnails or slide overview images.
     * <p>
     * Each associated image is simply a T that does not warrant (or require) a full
     * ImageServer, and most likely would never be analyzed.
     * 
     * @see #getAssociatedImage(String)
     * 
     * @return
     */
    public String[] getAssociatedImageNames() {
        return server.getAssociatedImageList().toArray(new String[0]);
    }

    // width, height
    public int[] getAssociatedImageSize(String name) {
        BufferedImage image = server.getAssociatedImage(name);
        if (image == null)
            return null;
        return new int[] { image.getWidth(), image.getHeight() };
    }

    /**
     * Get the image for a given associated image name.
     * 
     * @see #getAssociatedImageList()
     * 
     * @param name
     * @return PNG byte array
     */
    public byte[] getAssociatedImage(String name) {
        return bufferedImageToPNGBytes(server.getAssociatedImage(name));
    }

    /**
     * Get the default thumbnail for a specified z-slice and timepoint.
     * <p>
     * This should be the lowest resolution image that is available in the case of
     * the multiresolution
     * image, or else the full image. For large datasets, it may be used to
     * determine basic statistics or
     * histograms without requiring every pixel to be visited in the full resolution
     * image.
     * 
     * @param z
     * @param t
     * @return PNG byte array
     */
    public byte[] getDefaultThumbnail(int z, int t) {
        try {
            BufferedImage image = server.getDefaultThumbnail(z, t);
            return bufferedImageToPNGBytes(image);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Get an array of downsample factors supported by the server.
     *
     * @return
     */
    public double[] getPreferredDownsamples() {
        return server.getPreferredDownsamples();
    }

    /**
     * Number of resolutions for the image.
     * <p>
     * This is equivalent to {@code getPreferredDownsamples().length}.
     * 
     * @return
     */
    public int nResolutions() {
        return server.nResolutions();
    }

    /**
     * Get the downsample factor for a specified resolution level, where level 0 is
     * the full resolution image
     * and nResolutions() - 1 is the lowest resolution available.
     * 
     * @param level Resolution level, should be 0 &lt;= level &lt; nResolutions().
     * @return
     */
    public double getDownsampleForResolution(int level) {
        return server.getDownsampleForResolution(level);
    }

    /**
     * Get the image size for a specified resolution level, where level 0 is
     * the full resolution image
     * and nResolutions() - 1 is the lowest resolution available.
     * 
     * @param level Resolution level, should be 0 &lt;= level &lt; nResolutions().
     * @return [wdith, height]
     */
    public int[] getSizeForResolution(int level) {
        // https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/ImageServerMetadata.java#L940
        ImageServerMetadata.ImageResolutionLevel irl = meta.getLevel(level);
        return new int[] { irl.getWidth(), irl.getHeight() };
    }

    /**
     * Read a 2D(+C) image region for a specified z-plane and timepoint.
     * Coordinates and bounding box dimensions are in pixel units, at the full image
     * resolution
     * (i.e. when downsample = 1).
     * <p>
     * All channels are always returned.
     * 
     * @param downsample downsample factor for the region
     * @param x          x coordinate of the top left of the region bounding box
     * @param y          y coordinate of the top left of the region bounding box
     * @param width      bounding box width
     * @param height     bounding box height
     * @param z          index for the z-position
     * @param t          index for the timepoint
     * @return PNG byte array for the region being requested
     * @see https://github.com/qupath/qupath/blob/v0.5.1/qupath-core/src/main/java/qupath/lib/images/servers/AbstractTileableImageServer.java#L266
     * @implNote the newly added jars are for this method to work
     */
    public byte[] readRegion(double downsample, int x, int y, int width, int height, int z, int t) {
        // System.out.println(
        // String.format("readRegion: [downsample: %.4f, x: %d, y: %d, width: %d,
        // height: %d, z: %d, t: %d]",
        // downsample, x, y, width, height, z, t));
        try {
            BufferedImage image = server.readRegion(downsample, x, y, width, height, z,
                    t);
            return bufferedImageToPNGBytes(image);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Read a 2D(+C) image region for a specified z-plane and timepoint.
     * Coordinates and bounding box dimensions are in pixel units, at the full image
     * resolution
     * (i.e. when downsample = 1).
     * <p>
     * All channels are always returned.
     * 
     * @param level  Resolution level, should be 0 &lt;= level &lt; nResolutions()
     * @param x      x coordinate of the top left of the region bounding box
     * @param y      y coordinate of the top left of the region bounding box
     * @param width  bounding box width
     * @param height bounding box height
     * @param z      index for the z-position
     * @param t      index for the timepoint
     * @return PNG byte array for the region being requested
     * @see #readRegion(double, int, int, int, int, int, int)
     */
    public byte[] readRegion(int level, int x, int y, int width, int height, int z, int t) {
        return readRegion(getDownsampleForResolution(level), x, y, width, height, z, t);
    }

    /**
     * Get a tile for the request - ideally from the cache, but otherwise read it
     * and
     * then add it to the cache.
     * 
     * @param level  resolution level for the region
     * @param x      x coordinate of the top left of the region bounding box
     * @param y      y coordinate of the top left of the region bounding box
     * @param width  bounding box width
     * @param height bounding box height
     * @param z      index for the z-position
     * @param t      index for the timepoint
     * @return
     * @apiNote better not use this method, since it breaks `ImageServer<T>`
     *          interface, use `readRegion` instead
     */
    public byte[] getTile(int level, int x, int y, int width, int height, int z, int t) {
        // System.out.println(
        // String.format("getTile: [level: %d, x: %d, y: %d, width: %d, height: %d, z:
        // %d, t: %d]",
        // level, x, y, width, height, z, t));
        try {
            BufferedImage image = ((BioFormatsImageServer) server).readTile(TileRequest.createInstance(server, level,
                    ImageRegion.createInstance(
                            x, y, width, height, z, t)));
            return bufferedImageToPNGBytes(image);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    // seems like server's `readRegion` always returns RGB image, maybe no need to
    // convert to PNG, but how about `isRGB`?
    // https://github.com/qupath/qupath/blob/v0.5.1/qupath-core/src/main/java/qupath/lib/images/servers/AbstractTileableImageServer.java#L266
    private static byte[] bufferedImageToPNGBytes(BufferedImage image) {
        if (image == null)
            return null;

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            BufferedImage buffer = new BufferedImage(image.getWidth(null),
                    image.getHeight(null), BufferedImage.TYPE_INT_ARGB);

            buffer.createGraphics().drawImage(image, 0, 0, null);
            ImageIO.write(buffer, "PNG", baos);
            baos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return baos.toByteArray();
    }
}
