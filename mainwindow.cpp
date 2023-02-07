#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#define RUS_ACTION ("ru_RU")
#define EN_US_ACTION ("en_US")

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , numHiddenLayers(0)
  , numNeurons(nullptr)
  , ann(nullptr)
  , trainData(nullptr)
  , subTrainData(nullptr)
  , loadSampling(false)
{

  ui->setupUi(this);

  ui->rusLan->setObjectName(RUS_ACTION);
  ui->enUsLan->setObjectName(EN_US_ACTION);
  connect(ui->rusLan, &QAction::triggered, this, &MainWindow::changeLanguageSlot);
  connect(ui->enUsLan, &QAction::triggered, this, &MainWindow::changeLanguageSlot);

  translatorLanguage.load(":/FannInterface_ru", ".");
  qApp->installTranslator(&translatorLanguage);
}

MainWindow::~MainWindow()
{
  if(numNeurons)
    free(numNeurons);
  delete ui;
}

void MainWindow::selectionChanged()
{
  QCustomPlot* plotSender = qobject_cast<QCustomPlot*>(sender());
  if(plotSender->xAxis->selectedParts().testFlag(QCPAxis::spAxis) ||
     plotSender->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    plotSender->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  if(plotSender->yAxis->selectedParts().testFlag(QCPAxis::spAxis) ||
     plotSender->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    plotSender->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  for (int i = 0; i < plotSender->graphCount(); i++) {
    QCPGraph *graph = plotSender->graph(i);
    QCPPlottableLegendItem *item = plotSender->legend->itemWithPlottable(graph);
    if(item->selected() || graph->selected())
    {
      item->setSelected(true);
      graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
    }
  }
}

/*
* Установка числа скрытых слоёв
* Setting the number of hidden layers
*/

void MainWindow::on_spinBoxNumberLayers_valueChanged(int value)
{
  if((numHiddenLayers) && (numNeurons))
  {
    numNeurons = reinterpret_cast<unsigned int*>(realloc(numNeurons,(static_cast<unsigned int>(value)+2)* sizeof(unsigned int)));
  }
  numHiddenLayers = static_cast<unsigned int>(value);
  if(value>ui->cmbBoxSelectNeurons->count()) {
    for (int i = (ui->cmbBoxSelectNeurons->count()) + 1; i <= value; i++) {
      ui->cmbBoxSelectNeurons->addItem(tr("Слое %1").arg(QString::number(i)));
    }
  }
  else {
    for (int i = (ui->cmbBoxSelectNeurons->count()); i >= value; i--) {
      ui->cmbBoxSelectNeurons->removeItem(i);
    }
  }
}

/*
* Создать ИНС
* Create ANN
*/
void MainWindow::on_bttnCreateNetwork_clicked()
{

  numInput = static_cast<unsigned int>(ui->spinBoxNumberInput->value());
  numOutput = static_cast<unsigned int>(ui->spinBoxNumberOutput->value());

  if(!(numInput > 0 && numOutput >0)) {
    QMessageBox::critical(this, tr("Ошибка"),tr("Введены некорректные значения количества входов и выходов"));
    return;
  }
  if(ui->chckBoxLoadFromFile->isChecked()) {
    if(nullptr != ann)
        fann_destroy(ann);
    QString file_path = QFileDialog::getOpenFileName(this,tr("Открыть файл ИНС"),nullptr,"*.net");
    ann = fann_create_from_file(file_path.toUtf8().constData());
    ui->groupBoxTrainingSet->setEnabled(true);
  }
  else {
    numNeuronsStd = static_cast<unsigned int>(ui->spinBoxAllNeurons->value());
    if(nullptr != ann)
        fann_destroy(ann);
    numLayers = numHiddenLayers + 2;
    if(!ui->chckBoxAllOrAloneNeurons->isChecked())
    {
      if(nullptr == numNeurons)
          numNeurons = reinterpret_cast<unsigned int*>(calloc(numLayers, sizeof(unsigned int)));
      for (unsigned int i=1;i<numLayers-1;i++)
        numNeurons[i] = numNeuronsStd;
    }
    numNeurons[0] = numInput;
    numNeurons[numLayers-1] = numOutput;
    ann = fann_create_standard_array(numLayers, numNeurons);

    fann_set_activation_function_hidden(ann, fann_activationfunc_enum(ui->cmbBoxFuncActivationForLayers->currentIndex()));
    fann_set_activation_function_output(ann, fann_activationfunc_enum(ui->cmbBoxFuncActivationForOutputs->currentIndex()));
    ui->groupBoxTrainingSet->setEnabled(true);
  }
}

void MainWindow::on_chckBoxAllOrAloneNeurons_toggled(bool checked)
{
  if(checked == true) {
    ui->chckBoxAllOrAloneNeurons->setText(tr("Задавать количество нейронов всем слоям сразу"));
    ui->stackWdgtNeurons->setCurrentIndex(0);
  }
  else {
    ui->chckBoxAllOrAloneNeurons->setText(tr("Задавать количество нейронов каждому слою"));
    ui->stackWdgtNeurons->setCurrentIndex(1);
  }
}

void MainWindow::on_spinBoxNumberNeurons_valueChanged(int value)
{
  numNeurons[ui->cmbBoxSelectNeurons->currentIndex()+1] = static_cast<unsigned int>(value);
}

/*
 * Загрузить обучающую выборку
 * Load train data
 */

void MainWindow::on_bttnLoadSample_clicked()
{
  if(loadSampling) {
    foreach(auto *item, checkBoxes)
      delete item;
    foreach(auto *item, customPlots)
      delete item;
    ui->tabWgtGraphics->clear();
    checkBoxes.clear();
    customPlots.clear();
  }
  QString filePath = QFileDialog::getOpenFileName(this,tr("Открыть файл"),nullptr,"*.train");
  if(!filePath.isEmpty()){
      if(nullptr != trainData)
          fann_destroy_train(trainData);
      trainData = fann_read_train_from_file(filePath.toUtf8());
      QFileInfo fileInfo(filePath);

      ui->lineEditAnalysisFileName->setText(fileInfo.fileName());
      ui->lineEditAnalysisCountPair->setText(QString::number(trainData->num_data));
      ui->lineEditAnalysisCountInput->setText(QString::number(trainData->num_input));
      ui->lineEditAnalysisCountOutput->setText(QString::number(trainData->num_output));

      for (unsigned char j = 0; j < (trainData->num_output + trainData->num_input); j++) {
        QCheckBox *checkBox = new QCheckBox(ui->gridLayoutCheckBoxes->widget());
        if(j < trainData->num_input)
          checkBox->setText(tr("Вход %1").arg(QString::number(j+1)));
        else
          checkBox->setText(tr("Выход %1").arg(QString::number(j+1-trainData->num_input)));
        checkBox->setObjectName(QString::number(j));
        connect(checkBox, &QCheckBox::toggled, this, &MainWindow::slotChecked);
        ui->gridLayoutCheckBoxes->addWidget(checkBox);
        checkBoxes.append(checkBox);
      }

      for (unsigned char j = 0; j < trainData->num_output; j++) {
        QCustomPlot *customPlot = new QCustomPlot(this);

        customPlot->xAxis->setRange(0,trainData->num_data);
        customPlot->yAxis->setRange(-1,1);
        customPlot->xAxis->setLabel(tr("Данные"));
        customPlot->yAxis->setLabel(tr("Вход"));
        for(unsigned char i = 0; i < trainData->num_input+trainData->num_output; i++) {
          customPlot->addGraph();
          x.clear();
          y.clear();
          for (unsigned int k = 0; k < trainData->num_data; k++) {
            x.push_back(k);
            if(i<trainData->num_input)
              y.push_back(static_cast<double>(trainData->input[k][i]));
            else
              y.push_back(static_cast<double>(trainData->output[k][i]));
          }
          if(i < trainData->num_input)
            customPlot->graph(i)->setName(tr("Вход %1").arg(i+1));
          else
            customPlot->graph(i)->setName(tr("Выход %1").arg(i+1-trainData->num_input));
          customPlot->legend->setVisible(true);
          QPen penPlot;
          penPlot.setWidth(3);
          penPlot.setColor(QColor((rand()%255),rand()%255,(rand()%255)));
          customPlot->graph(i)->addData(x,y);
          customPlot->graph(i)->setVisible(false);
          customPlot->graph(i)->setPen(penPlot);
        }
        customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);
        customPlot->legend->setSelectableParts(QCPLegend::spItems);
        customPlot->setObjectName(QString::number(j));
        connect(customPlot, &QCustomPlot::mouseDoubleClick, this, &MainWindow::doubleCkickSlot);
        connect(customPlot, &QCustomPlot::mousePress, this, &MainWindow::pressEventSlot);
        connect(customPlot, &QCustomPlot::mouseRelease, this, &MainWindow::releaseEventSlot);
        connect(customPlot, &QCustomPlot::selectionChangedByUser, this, &MainWindow::selectionChanged);
        ui->tabWgtGraphics->insertTab(j,customPlot,tr("График %1").arg(j+1));
        customPlots.append(customPlot);
      }

      for(unsigned char idGrapfics = 0; idGrapfics < ui->tabWgtGraphics->count(); idGrapfics++) {
        QCustomPlot * plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(idGrapfics));
        bool isVisiblePlotOnGrapfic = false;
        for(unsigned char iPlotOnGrapfic = 0; iPlotOnGrapfic < plot->graphCount(); iPlotOnGrapfic++) {
          if(iPlotOnGrapfic == idGrapfics + trainData->num_input)
              isVisiblePlotOnGrapfic = true;
          else
              isVisiblePlotOnGrapfic = false;
          plot->graph(iPlotOnGrapfic)->setVisible(isVisiblePlotOnGrapfic);
          plot->replot();
        }
      }

      QCustomPlot * plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(ui->tabWgtGraphics->currentIndex()));
      for(int i = 0; i < ui->gridLayoutCheckBoxes->count(); i++) {
        QCheckBox *box = qobject_cast<QCheckBox*>(ui->gridLayoutCheckBoxes->itemAt(i)->widget());
        box->setChecked(plot->graph(i)->visible());
      }

      ui->groupBoxTraining->setEnabled(true);
      ui->bttnEducate->setEnabled(false);
      ui->bttnDisplayGraphic->setEnabled(false);
      ui->bttnClearResultEducate->setEnabled(false);
      loadSampling = true;
  }
}

/*
 *Отображаем графики соответствующие чекбоксам
 */
void MainWindow::slotChecked(bool state)
{
  QCheckBox *box = qobject_cast<QCheckBox*>(sender());
  QCustomPlot * plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(ui->tabWgtGraphics->currentIndex()));
  plot->graph(box->objectName().toInt())->setVisible(state);
  plot->replot();
}

/*
 * Двойной щелчок мыши на графике
 */

void MainWindow::doubleCkickSlot(QMouseEvent *event)
{
  Q_UNUSED(event)
  QCustomPlot* plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->currentWidget());
  plot->xAxis->setRange(0,trainData->num_data);
  plot->yAxis->setRange(-1,1);
  plot->replot();
}
/*
* Клик ПКМ на графике
*/
void MainWindow::pressEventSlot(QMouseEvent *event)
{
  QCustomPlot * plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(ui->tabWgtGraphics->currentIndex()));
  if(event->button() & Qt::LeftButton)
  {
    plot->setSelectionRectMode(QCP::srmZoom);
  }
  if(event->button() & Qt::RightButton)
  {
    plot->setSelectionRectMode(QCP::srmNone);
    this->setCursor(Qt::ClosedHandCursor);
  }
}

void MainWindow::releaseEventSlot(QMouseEvent *event)
{
  if(event->button() & Qt::RightButton)
  {
    this->setCursor(Qt::ArrowCursor);
  }
}

void MainWindow::changeLanguageSlot()
{
  QAction *actionSender = qobject_cast<QAction*>(sender());
  if(!actionSender)
    return;
  if(actionSender->objectName() == RUS_ACTION){
    translatorLanguage.load(":/FannInterface_ru", ".");
    qApp->installTranslator(&translatorLanguage);
  }
  else if(actionSender->objectName() == EN_US_ACTION){
    translatorLanguage.load(":/FannInterface_en_US", ".");
    qApp->installTranslator(&translatorLanguage);
  }
}

void MainWindow::retranslateGUI()
{
  for(int i = 0, it = ui->tabWgtGraphics->count(); i != it; ++i){
    QByteArray ba = tr("График %1").arg(i+1).toUtf8();
    ui->tabWgtGraphics->setTabText(i, QCoreApplication::translate("MainWindow", ba.data(), nullptr));
    QCustomPlot *plot = static_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(i));
    for(int j = 0, jt = plot->graphCount(); j != jt; ++j){
      if(static_cast<unsigned char>(j) < trainData->num_input){
        QByteArray ba = tr("Вход %1").arg(j+1).toUtf8();
        plot->graph(j)->setName(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
      }
      else
      {
        QByteArray ba = tr("Выход %1").arg(j+1 - trainData->num_input).toUtf8();
        plot->graph(j)->setName(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
      }

//    for(int j = 0, jt = newGraphs.count(); j != jt; ++j){
      if(static_cast<unsigned char>(j) >= (trainData->num_input + trainData->num_output)){
        if(numNeurons){
          QByteArray ba = tr("Вых.%1_%2_%3x%4").arg(j-trainData->num_input - trainData->num_output +1)
                          .arg(ui->cmbBoxTrainAlgorithm->currentIndex())
                          .arg(ui->spinBoxNumberLayers->value())
                          .arg(numNeurons[1]).toUtf8();
          plot->graph(j)->setName(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
        }
        else{
          QByteArray ba = tr("Вых.%1_%2_%3x%4").arg(j-trainData->num_input - trainData->num_output +1)
                          .arg(ui->cmbBoxTrainAlgorithm->currentIndex())
                          .arg(ui->spinBoxNumberLayers->value())
                          .arg(ui->spinBoxNumberNeurons->value()).toUtf8();
          plot->graph(j)->setName(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
        }
      }
      ba.clear();
      ba = "Данные";

      plot->xAxis->setLabel(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
      ba = "Вход";
      plot->yAxis->setLabel(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
      plot->replot();
    }
  }
  for(unsigned char i = 0, it = ui->gridLayoutCheckBoxes->count(); i != it; ++i) {
    QCheckBox *checkBox = qobject_cast<QCheckBox *> (ui->gridLayoutCheckBoxes->itemAt(i)->widget());
    if(i < trainData->num_input){
      QByteArray ba = tr("Вход %1").arg(i+1).toUtf8();
      checkBox->setText(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
    }
    else
    {
      QByteArray ba = tr("Выход %1").arg(i+1 - trainData->num_input).toUtf8();
      checkBox->setText(QCoreApplication::translate("MainWindow", ba.data(), nullptr));
    }
  }

}

/*
* Отображаем чекбоксы соответствующие графикам
*/
void MainWindow::on_tabWgtGraphics_currentChanged(int index)
{
  ui->groupBoxGraphics->setTitle(tr("График %1").arg(index+1));
  QCustomPlot * plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(index));
  for(int i = 0; i < ui->gridLayoutCheckBoxes->count(); i++) {
    QCheckBox *checkBox = qobject_cast<QCheckBox *> (ui->gridLayoutCheckBoxes->itemAt(i)->widget());
    checkBox->setChecked(plot->graph(i)->visible());
  }
}
/*
* Задать частичную выборку
*/
void MainWindow::on_chckBoxSubsample_stateChanged(int state)
{
  if(state)
    ui->bttnEducate->setText(tr("Обучить на частичной выборке"));
  else
    ui->bttnEducate->setText(tr("Обучить на полной выборке"));
  ui->groupBoxParametersSubsample->setEnabled(state);
}
/*
* Активируем кнопку Обучить на ... выборке
*/
void MainWindow::on_lineEditLearningErrorValue_editingFinished()
{
  if(!ui->lineEditMaxCountEras->text().isEmpty()        &&
     !ui->lineEditOutputReportEras->text().isEmpty()     &&
     !ui->lineEditLearningErrorValue->text().isEmpty())
  {
    ui->bttnEducate->setEnabled(true);
  }
  else{
      ui->bttnEducate->setEnabled(false);
  }
}

/*
* Обучить на ... выборке
*/
void MainWindow::on_bttnEducate_clicked()
{
  if(ui->chckBoxSubsample->isChecked()){
      startNumTrain = ui->lineEditIntervalFrom->text().toUInt();
      finishNumTrain = ui->lineEditIntervalBefore->text().toUInt();
      subTrainData = fann_subset_train_data(trainData, startNumTrain, finishNumTrain);
  }
  else {
      finishNumTrain = fann_length_train_data (trainData);
      subTrainData = fann_duplicate_train_data(trainData);
  }

  fann_stopfunc_enum stopFunc = fann_stopfunc_enum(ui->cmbBoxTrainStop->currentIndex());
  fann_set_train_stop_function(ann, stopFunc);
  float bitFailLimit = 0.01f;
  fann_set_bit_fail_limit(ann, bitFailLimit);

  fann_train_enum trainAlgorithm = fann_train_enum(ui->cmbBoxTrainAlgorithm->currentIndex());
  fann_set_training_algorithm(ann, trainAlgorithm);
  if(ui->chckBoxSetWeight->isChecked())
    fann_init_weights(ann, subTrainData);
  else
    fann_randomize_weights(ann, -0.1f, 0.1f);

  printf("Training network.\n");
  const float desiredError = ui->lineEditLearningErrorValue->text().toFloat();
  const unsigned int maxEpochs = ui->lineEditMaxCountEras->text().toUInt();
  const unsigned int epochsBetweenReports = ui->lineEditOutputReportEras->text().toUInt();
  //если FANN_STOPFUNC_MSE, то desired_error - это MSE,
  //если FANN_STOPFUNC_BIT, то desired_error - это кол-во BIT,
  fann_train_on_data(ann, subTrainData, maxEpochs, epochsBetweenReports, desiredError);

  ui->bttnEducate->setEnabled(false);
  ui->bttnDisplayGraphic->setEnabled(true);
  ui->bttnClearResultEducate->setEnabled(false);
}
/*
* Отображение на графике
*/
void MainWindow::on_bttnDisplayGraphic_clicked()
{
  //вывод на графики
  fann_type *calcOut;
  fann_type *input;

  for (int k = 0; k < ui->tabWgtGraphics->count(); k++) {
    QCustomPlot * plot = qobject_cast<QCustomPlot*>(ui->tabWgtGraphics->widget(k));
    for(unsigned int j = trainData->num_input; j < trainData->num_input+trainData->num_output; j++) {
      if(plot->graph(static_cast<int>(j))->visible()) {
        QCPGraph* graphic = plot->addGraph();
        newGraphs.append(graphic);
        int numGraph = plot->graphCount()-1;
        QPen penPlot;
        penPlot.setWidth(1);
        penPlot.setColor(QColor((rand()%255),rand()%255,(rand()%255)));
        plot->graph(numGraph)->setPen(penPlot);
        if(numNeurons)
          plot->graph(numGraph)->setName(tr("Вых.%1_%2_%3x%4").arg(j-trainData->num_input+1)
                                                                  .arg(ui->cmbBoxTrainAlgorithm->currentIndex())
                                                                  .arg(ui->spinBoxNumberLayers->value())
                                                                  .arg(numNeurons[1]));
        else
          plot->graph(numGraph)->setName(tr("Вых.%1_%2_%3x%4").arg(j-trainData->num_input+1)
                                                                  .arg(ui->cmbBoxTrainAlgorithm->currentIndex())
                                                                  .arg(ui->spinBoxNumberLayers->value())
                                                                  .arg(ui->spinBoxNumberNeurons->value()));
        for(unsigned int i=startNumTrain; i<finishNumTrain; i++) {
          input = fann_get_train_output(subTrainData,i);
          calcOut = fann_run(ann, input);
          plot->graph(numGraph)->addData(i,static_cast<double>(calcOut[j-trainData->num_input]));
        }
      }
    }
    plot->replot();
  }
  fann_destroy_train(subTrainData);

  ui->bttnEducate->setEnabled(true);
  ui->bttnDisplayGraphic->setEnabled(false);
  ui->bttnClearResultEducate->setEnabled(true);
}

void MainWindow::on_chckBoxAllOrAloneNeurons_stateChanged(int state)
{
  if(state)
  {
    if(numNeurons)
    {
      free(numNeurons);
      numNeurons = nullptr;
    }
    numLayers = numHiddenLayers + 2;
    numNeurons = reinterpret_cast<unsigned int*>(calloc(numLayers,numLayers*sizeof (unsigned int)));
    numInput = static_cast<unsigned int>(ui->spinBoxNumberInput->value());
    numOutput = static_cast<unsigned int>(ui->spinBoxNumberOutput->value());
    numNeurons[0] = numInput;
    numNeurons[numLayers-1] = numOutput;
  }
  else
    free(numNeurons);
}

void MainWindow::on_cmbBoxSelectNeurons_currentIndexChanged(int index)
{
  unsigned char nNeurosInCurrentLayer = 0;
  ui->spinBoxNumberNeurons->setValue(nNeurosInCurrentLayer);
  if(   nullptr != numNeurons
     && ui->chckBoxAllOrAloneNeurons->isChecked()
     && nNeurosInCurrentLayer>0)
      numNeurons[index+1] = nNeurosInCurrentLayer;
}

void MainWindow::on_bttnClearResultEducate_clicked()
{
  unsigned int standatdGraphsCount = trainData->num_input + trainData->num_output;
  foreach(auto*plot, customPlots) {
    unsigned int allGraphs = static_cast<unsigned int>(plot->graphCount());
    unsigned int diff = allGraphs - standatdGraphsCount;
    for(unsigned int i = 0; i < diff; i++) {
      plot->removeGraph(newGraphs[0]);
      newGraphs.removeFirst();
    }
    plot->replot();
  }

  ui->bttnEducate->setEnabled(true);
  ui->bttnDisplayGraphic->setEnabled(false);
  ui->bttnClearResultEducate->setEnabled(false);
}

void MainWindow::changeEvent(QEvent *event)
{
  if(event->type() == QEvent::LanguageChange){
    ui->retranslateUi(this);
    retranslateGUI();
  }
}
