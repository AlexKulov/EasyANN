#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <QVector>
#include <QMap>
#include <QWidget>
#include <QSharedPointer>
#include <QTranslator>

#include "fann/include/floatfann.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();


private slots:
  void selectionChanged();
  void slotChecked(bool state);
  void doubleCkickSlot(QMouseEvent* event);
  void pressEventSlot(QMouseEvent* event);
  void releaseEventSlot(QMouseEvent* event);
  void changeLanguageSlot();
  void retranslateGUI();

  void on_spinBoxNumberLayers_valueChanged(int arg1);
  void on_bttnCreateNetwork_clicked();
  void on_chckBoxAllOrAloneNeurons_toggled(bool checked);
  void on_spinBoxNumberNeurons_valueChanged(int arg1);
  void on_bttnLoadSample_clicked();
  void on_tabWgtGraphics_currentChanged(int index);
  void on_chckBoxSubsample_stateChanged(int arg1);
  void on_lineEditLearningErrorValue_editingFinished();
  void on_bttnEducate_clicked();
  void on_bttnDisplayGraphic_clicked();
  void on_chckBoxAllOrAloneNeurons_stateChanged(int arg1);
  void on_cmbBoxSelectNeurons_currentIndexChanged(int index);

  void on_bttnClearResultEducate_clicked();

private:
  Ui::MainWindow *ui;
  unsigned int    numLayers, numHiddenLayers;
  unsigned int    *numNeurons;
  unsigned int    numNeuronsStd;
  unsigned int    numOutput, numInput;
  struct fann     *ann;

  struct          fann_train_data *trainData, *subTrainData;

  unsigned int startNumTrain = 0;
  unsigned int finishNumTrain = 0;
  QVector<double> x,y;
  bool loadSampling;

  QList<QCheckBox*> checkBoxes;
  QList<QCustomPlot*> customPlots;
  QList<QCPGraph *> newGraphs;
  QTranslator translatorLanguage;

protected:
  void changeEvent(QEvent *event) override;
};
#endif // MAINWINDOW_H
