#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCaptionActor2D.h>
#include <vtkCamera.h>
#include <vtkDataSetMapper.h>
#include <vtkElevationFilter.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>

int main(int, char*[])
{
  vtkNew<vtkNamedColors> colors;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(16);
  sphere->SetThetaResolution(12);

  vtkNew<vtkElevationFilter> colorIt;
  colorIt->SetInputConnection(sphere->GetOutputPort());
  colorIt->SetLowPoint(0, -0.5, 0);
  colorIt->SetHighPoint(0, 0.5, 0);

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(colorIt->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();
  actor->GetProperty()->SetEdgeColor(0, 0, 0);
  actor->GetProperty()->SetLineWidth(1.0);

  renderer->AddActor(actor);

  // x, y, z axes with arrow tips: red, green, blue
  vtkNew<vtkAxesActor> axes;
  axes->SetShaftTypeToCylinder();
  axes->SetTipTypeToCone();
  axes->SetTotalLength(1.0, 1.0, 1.0);
  axes->SetCylinderRadius(0.01);
  axes->SetConeRadius(0.2);
  double const axisColors[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  vtkProperty* shafts[3] = {axes->GetXAxisShaftProperty(),
                            axes->GetYAxisShaftProperty(),
                            axes->GetZAxisShaftProperty()};
  vtkProperty* tips[3] = {axes->GetXAxisTipProperty(),
                          axes->GetYAxisTipProperty(),
                          axes->GetZAxisTipProperty()};
  vtkCaptionActor2D* labels[3] = {axes->GetXAxisCaptionActor2D(),
                                  axes->GetYAxisCaptionActor2D(),
                                  axes->GetZAxisCaptionActor2D()};
  for (int i = 0; i < 3; ++i)
  {
    shafts[i]->SetColor(axisColors[i][0], axisColors[i][1], axisColors[i][2]);
    tips[i]->SetColor(axisColors[i][0], axisColors[i][1], axisColors[i][2]);
    labels[i]->GetCaptionTextProperty()->SetColor(
        axisColors[i][0], axisColors[i][1], axisColors[i][2]);
  }
  renderer->AddActor(axes);
  renderer->SetBackground(colors->GetColor3d("SlateGray").GetData());
  renWin->SetSize(640, 480);
  renWin->SetWindowName("ColoredSphere");

  renWin->Render();

  // interact with data
  iren->Start();

  return EXIT_SUCCESS;
}