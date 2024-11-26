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

    // TODO: enumerate with type
    auto type = reader.getPixelType();
    assert(type == Reader::PixelType::UINT8);

    auto width = reader.getSizeX();
    auto height = reader.getSizeY();
    auto depth = reader.getSizeZ();
    auto sx = reader.getPhysSizeX();
    auto sy = reader.getPhysSizeY();
    auto sz = reader.getPhysSizeZ();
    auto bytesPerPixel = reader.getBytesPerPixel();
    auto rgbChannelCount = reader.getRGBChannelCount();
    auto planeSize = reader.getPlaneSize();

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
    imageImport->SetDataSpacing(sx, sy, sz);
    imageImport->SetDataOrigin(0, 0, 0);
    imageImport->SetWholeExtent(0, width - 1, 0, height - 1, 0, depth - 1);
    imageImport->SetDataExtentToWholeExtent();
    imageImport->SetNumberOfScalarComponents(rgbChannelCount);
    imageImport->SetDataScalarTypeToUnsignedChar();
    imageImport->SetImportVoidPointer((void*)buffer.get());
    imageImport->Update();

    vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
    mapper->SetInputData(imageImport->GetOutput());
    mapper->Update();
    mapper->AutoAdjustSampleDistancesOn();
    mapper->SetBlendModeToMaximumIntensity(); // MIP

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

    vtkNew<vtkVolume> volume;
    volume->SetMapper(mapper);
    volume->SetProperty(volumeProperty);

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