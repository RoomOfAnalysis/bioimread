import java.io.IOException;
import loci.common.services.ServiceFactory;
import loci.common.xml.XMLTools;
import loci.formats.ImageReader;
import loci.formats.meta.IMetadata;
import loci.formats.meta.MetadataRetrieve;
import loci.formats.services.OMEXMLService;
import loci.formats.FormatException;

public class bfwrapper
{
    // https://bio-formats.readthedocs.io/en/v8.0.0/developers/file-reader.html
    // https://github.com/ome/bioformats/blob/develop/components/bio-formats-tools/src/loci/formats/tools/ImageInfo.java#L522
    public String getMetadata(String filePath)
    {
        try
        {
            ImageReader reader = new ImageReader();
            ServiceFactory factory = new ServiceFactory();
            OMEXMLService service = factory.getInstance(OMEXMLService.class);
            IMetadata meta = service.createOMEXMLMetadata();
            reader.setMetadataStore(meta);
            reader.setId(filePath);
            String info = Thread.currentThread().getName() +
              ": path=" + filePath +
              ", seriesCount=" + reader.getSeriesCount() +
              ", imageCount=" + reader.getImageCount() +
              ", sizeX=" + reader.getSizeX() +
              ", sizeY=" + reader.getSizeY() +
              ", sizeZ=" + reader.getSizeZ() +
              ", sizeC=" + reader.getSizeC() +
              ", sizeT=" + reader.getSizeT() +
              ", imageName=" + meta.getImageName(0);
            reader.close();
            return info;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return "Error: " + e.getMessage();
        }
    }

    public String getOMEXML(String filePath)
    {
        try
        {
            ImageReader reader = new ImageReader();
            ServiceFactory factory = new ServiceFactory();
            OMEXMLService service = factory.getInstance(OMEXMLService.class);
            IMetadata meta = service.createOMEXMLMetadata();
            reader.setMetadataStore(meta);
            reader.setId(filePath);
            String xml = service.getOMEXML((MetadataRetrieve) meta);
            XMLTools.indentXML(xml, 3, true);
            reader.close();
            return xml;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return "Error: " + e.getMessage();
        }
    }

    // TODO: get image bytes
}