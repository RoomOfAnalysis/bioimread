#include "../bfwrapper/reader.hpp"

#include <cassert>

#include <vtkNew.h>
#include <vtkImageImport.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkNamedColors.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>

// ./volume_viewer.exe xxx.tiff 0 0 0
int main(int argc, char* argv[])
{
    assert(argc > 1);

    int const series = (argc == 2) ? 0 : std::stoi(argv[2]);
    int const channel = (argc <= 3) ? 0 : std::stoi(argv[3]);
    int const timepoint = (argc <= 4) ? 0 : std::stoi(argv[4]);

    Reader reader;
    reader.open(argv[1]);
    reader.setSeries(series);

    auto type = reader.getPixelType();
    if (type == Reader::PixelType::BIT || type == Reader::PixelType::INT8 || type == Reader::PixelType::UINT32)
    {
        std::cerr << "Error: " << Reader::pixelTypeStr(type) << " not supported by VTK";
        return -1;
    }

    auto width = reader.getSizeX();
    auto height = reader.getSizeY();
    auto depth = reader.getSizeZ();
    auto sx = reader.getPhysSizeX();
    auto sy = reader.getPhysSizeY();
    auto sz = reader.getPhysSizeZ();
    // FIXME: some image sz == 0...
    if (sz == 0) sz = sx;
    auto bytesPerPixel = reader.getBytesPerPixel();
    auto rgbChannelCount = reader.getRGBChannelCount();
    auto planeSize = reader.getPlaneSize();

    std::cout << "pixel type: " << Reader::pixelTypeStr(type) << ", width: " << width << ", height: " << height
              << ", depth: " << depth << ", physicalX: " << sx << ", physicalY: " << sy << ", physicalZ: " << sz
              << ", bytesPerPixel: " << bytesPerPixel << ", rgbChannelCount: " << rgbChannelCount
              << ", planeSize: " << planeSize << std::endl;

    reader.getPlane(reader.getPlaneIndex(0, channel, timepoint));
    auto lut = reader.get8BitLut();

    size_t dataSize = (size_t)planeSize * depth;
    auto buffer = std::make_unique<unsigned char[]>(dataSize);
    auto ptr = buffer.get();
    for (auto i = 0; i < depth; i++)
    {
        auto plane = reader.getPlane(reader.getPlaneIndex(i, channel, timepoint));
        std::memcpy(ptr, plane.get(), planeSize);
        ptr += planeSize;
    }

    vtkNew<vtkImageImport> imageImport;
    switch (type)
    {
    case Reader::PixelType::UINT8:
        imageImport->SetDataScalarTypeToUnsignedChar();
        break;
    case Reader::PixelType::INT16:
        imageImport->SetDataScalarTypeToShort();
        break;
    case Reader::PixelType::UINT16:
        imageImport->SetDataScalarTypeToUnsignedShort();
        break;
    case Reader::PixelType::INT32:
        imageImport->SetDataScalarTypeToInt();
        break;
    case Reader::PixelType::FLOAT:
        imageImport->SetDataScalarTypeToFloat();
        break;
    case Reader::PixelType::DOUBLE:
        imageImport->SetDataScalarTypeToDouble();
        break;
    default:
        std::cerr << "Error: " << Reader::pixelTypeStr(type) << " not supported by VTK";
        return -1;
    }
    imageImport->SetDataSpacing(sx, sy, sz);
    imageImport->SetDataOrigin(0, 0, 0);
    imageImport->SetWholeExtent(0, width - 1, 0, height - 1, 0, depth - 1);
    imageImport->SetDataExtentToWholeExtent();
    imageImport->SetNumberOfScalarComponents(rgbChannelCount);
    imageImport->SetImportVoidPointer((void*)buffer.get());
    imageImport->Update();

    vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
    mapper->SetInputData(imageImport->GetOutput());
    mapper->Update();
    mapper->AutoAdjustSampleDistancesOn();
    mapper->SetBlendModeToMaximumIntensity(); // MIP

    vtkNew<vtkVolume> volume;
    volume->SetMapper(mapper);

    if (lut)
    {
        vtkNew<vtkColorTransferFunction> colorTransferFunction;
        colorTransferFunction->RemoveAllPoints();
        for (auto i = 0; i < lut->size(); i++)
        {
            auto [r, g, b] = (*lut)[i];
            colorTransferFunction->AddRGBPoint(i, r / 255., g / 255., b / 255.);
        }

        vtkNew<vtkPiecewiseFunction> scalarOpacity;
        scalarOpacity->AddSegment(0, 1.0, 256, 0.1);

        vtkNew<vtkVolumeProperty> volumeProperty;
        volumeProperty->SetInterpolationTypeToLinear();
        volumeProperty->SetColor(colorTransferFunction);
        volumeProperty->SetScalarOpacity(scalarOpacity);
        volume->SetProperty(volumeProperty);
    }

    vtkNew<vtkNamedColors> colors;

    vtkNew<vtkRenderer> renderer;
    renderer->AddVolume(volume);
    renderer->SetBackground(colors->GetColor3d("black").GetData());
    renderer->ResetCamera();

    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->SetSize(800, 600);
    renderWindow->AddRenderer(renderer);
    renderWindow->SetWindowName("VolumeViewer");

    vtkNew<vtkInteractorStyleTrackballCamera> style;

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renderWindow);
    interactor->SetInteractorStyle(style);

    renderWindow->Render();
    interactor->Start();

    return 0;
}