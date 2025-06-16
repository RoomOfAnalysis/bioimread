import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.sql.Time;
import java.util.List;
import java.util.Locale;
import java.util.stream.Collectors;

import javax.imageio.ImageReader;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ij.ImagePlus;
import loci.common.DebugTools;
import loci.common.Region;
import loci.plugins.BF;
import loci.plugins.in.ImporterOptions;
import qupath.lib.common.GeneralTools;
import qupath.lib.images.servers.bioformats.BioFormatsServerBuilder;
import qupath.lib.images.servers.bioformats.BioFormatsImageServer;
import qupath.lib.images.servers.ImageServer;
import qupath.lib.images.servers.ImageServers;
import qupath.lib.images.servers.PixelCalibration;
import qupath.lib.images.servers.PixelType;
import qupath.lib.projects.Project;
import qupath.lib.projects.ProjectIO;
import qupath.lib.projects.ProjectImageEntry;
import qupath.lib.regions.RegionRequest;

public class qpwrapper implements AutoCloseable {
    private BioFormatsServerBuilder builder;
    private ImageServer<BufferedImage> server; // https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/ImageServer.java

    public qpwrapper(String path) {
        try {
            builder = new BioFormatsServerBuilder();
            server = builder.buildServer(Paths.get(path).toUri());
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
        return server.getMetadata().toString();
    }

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
        return server.getPixelCalibration().getPixelWidthMicrons() * 1000.0 * server.getWidth();
    }

    // mm
    public double getPhysSizeY() {
        return server.getPixelCalibration().getPixelHeightMicrons() * 1000.0 * server.getHeight();
    }

    // mm
    public double getPhysSizeZ() {
        return server.getPixelCalibration().getZSpacingMicrons() * 1000.0 * server.nZSlices();
    }

    // s
    public double getPhysSizeT() {
        return server.getPixelCalibration().getTimeUnit().toSeconds(1) * server.nTimepoints();
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
        return server.getMetadata().getPixelType().getBitsPerPixel();
    }

    /**
     * Retrieves how many bytes per pixel the current plane or section has.
     * {@link #getPixelType()}.
     * 
     * @return the number of bytes per pixel.
     * @see https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/PixelType.java#L108
     */
    public int getBytesPerPixel() {
        return server.getMetadata().getPixelType().getBytesPerPixel();
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

    public List<String> getAssociatedImageList() {
        return server.getAssociatedImageList();
    }

    public BufferedImage getAssociatedImage(String name) {
        return server.getAssociatedImage(name);
    }

    public BufferedImage getDefaultThumbnail(int z, int t) throws IOException {
        return server.getDefaultThumbnail(z, t);
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
     * @return pixels for the region being requested
     * @throws IOException
     * @see #readRegion(RegionRequest)
     */
    public BufferedImage readRegion(double downsample, int x, int y, int width, int height, int z, int t)
            throws IOException {
        return server.readRegion(downsample, x, y, width, height, z, t);
    }
}
