/*!
 * @file seqwidget.cpp
 * @brief Implements the SeqWidget GUI class.
 *
 * @section LICENSE
 *
 *      Copyright 2009 - 2016 <qmidiarp-devel@lists.sourceforge.net>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <QLabel>
#include <QBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>

#include "seqwidget.h"

#include "pixmaps/seqrecord.xpm"


#ifdef APPBUILD
SeqWidget::SeqWidget(MidiSeq *p_midiWorker, GlobStore *p_globStore,
    int portCount, bool compactStyle,
    bool mutedAdd, bool inOutVisible, const QString& p_name):
    InOutBox(p_midiWorker, p_globStore, portCount, compactStyle, inOutVisible, p_name),
    midiWorker(p_midiWorker)
{
#else
SeqWidget::SeqWidget(
    bool compactStyle,
    bool mutedAdd, bool inOutVisible):
    InOutBox(compactStyle, inOutVisible, "Seq:"),
    midiWorker(NULL)
{
#endif

    connect(muteOutAction, SIGNAL(toggled(bool)), this, 
            SLOT(setMuted(bool)));


    // group box for sequence setup
    QGroupBox *seqBox = new QGroupBox(tr("Sequence"));

    screen = new SeqScreen;
    screen->setToolTip(
        tr("Right button to mute points, left button to draw custom wave"));
    screen->setMinimumHeight(SEQSCR_MIN_H);
    connect(screen, SIGNAL(mouseEvent(double, double, int, int)), this,
            SLOT(mouseEvent(double, double, int, int)));

    cursor = new Cursor('S');

    deferChangesAction = new QAction("D", this);
    deferChangesAction->setToolTip(tr("Defer mute, velocity, note length and transpose to pattern end"));
    deferChangesAction->setCheckable(true);
    connect(deferChangesAction, SIGNAL(toggled(bool)), this, SLOT(updateDeferChanges(bool)));

    QToolButton *deferChangesButton = new QToolButton;
    deferChangesButton->setDefaultAction(deferChangesAction);
    deferChangesButton->setFixedSize(20, 20);

    loopBox = new QComboBox(seqBox);
    QStringList names;
    names.clear();
    names << "->_>" << " <_<-" << "->_<" << " >_<-" << "->_|" << " |_<-" << "RANDM";
    loopBox->insertItems(0, names);
    loopBox->setCurrentIndex(0);
    loopBox->setToolTip(tr("Loop, bounce or play once going forward or backward"));
    loopBox->setMinimumContentsLength(5);
    connect(loopBox, SIGNAL(activated(int)), this,
            SLOT(updateLoop(int)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("LoopMode", loopBox, 6);
#endif
    QLabel *recordButtonLabel = new QLabel(tr("Re&cord"));
    recordAction = new QAction(QPixmap(seqrecord_xpm), tr("Re&cord"), this);
    recordAction->setToolTip(tr("Record step by step"));
    recordAction->setCheckable(true);
    QToolButton *recordButton = new QToolButton;
    recordButton->setDefaultAction(recordAction);
    recordButtonLabel->setBuddy(recordButton);
    connect(recordAction, SIGNAL(toggled(bool)), this, SLOT(setRecord(bool)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("RecordToggle", recordButton, 3);
#endif

    QLabel *resBoxLabel = new QLabel(tr("&Resolution"),
            seqBox);
    resBox = new QComboBox(seqBox);
    resBoxLabel->setBuddy(resBox);
    names.clear();
    names << "1" << "2" << "4" << "8" << "16";
    resBox->insertItems(0, names);
    resBox->setCurrentIndex(2);
    resBoxIndex = 2;
    resBox->setToolTip(
            tr("Resolution (notes/beat): Number of notes produced every beat"));
    resBox->setMinimumContentsLength(3);
    connect(resBox, SIGNAL(activated(int)), this,
            SLOT(updateRes(int)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("Resolution", resBox, 4);
#endif

    QLabel *sizeBoxLabel = new QLabel(tr("&Length"));
    sizeBox = new QComboBox;
    sizeBoxLabel->setBuddy(sizeBox);
    names.clear();
    names << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "16" << "32";
    sizeBox->insertItems(0, names);
    sizeBox->setCurrentIndex(3);
    sizeBoxIndex = 3;
    sizeBox->setToolTip(tr("Length of Sequence in beats"));
    sizeBox->setMinimumContentsLength(3);
    connect(sizeBox, SIGNAL(activated(int)), this,
            SLOT(updateSize(int)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("Size", sizeBox, 5);
#endif

    dispSignalMapper = new QSignalMapper;
    QLabel *dispLabel[4];
    QString dispText[4] = {tr("&F"), tr("&U"), tr("&M"), tr("&L")};
    QString dispToolTip[4] = {tr("Full"), tr("Upper"), tr("Mid"), tr("Lower")};

    QHBoxLayout *dispBoxLayout = new QHBoxLayout;

    dispBoxLayout->addWidget(new QLabel(tr("Display")));
    dispBoxLayout->addStretch(10);
    for (int l1 = 0; l1 < 4; l1++) {
        dispLabel[l1] = new QLabel(dispText[l1]);
        dispVert[l1] = new QCheckBox;
        connect(dispVert[l1], SIGNAL(toggled(bool)), dispSignalMapper, SLOT(map()));
        dispSignalMapper->setMapping(dispVert[l1], l1);
        dispVert[l1]->setAutoExclusive(true);
        dispLabel[l1]->setBuddy(dispVert[l1]);
        dispVert[l1]->setToolTip(dispToolTip[l1]);
        dispBoxLayout->addWidget(dispLabel[l1]);
        dispBoxLayout->addWidget(dispVert[l1]);
    }
    dispBoxLayout->addStretch();

    dispVert[0]->setChecked(true);
    dispVertIndex = 0;
    connect(dispSignalMapper, SIGNAL(mapped(int)),
             this, SLOT(updateDispVert(int)));

    velocity = new Slider(0, 127, 1, 8, 64, Qt::Horizontal,
            tr("Veloc&ity"), this);
    connect(velocity, SIGNAL(valueChanged(int)), this,
            SLOT(updateVelocity(int)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("Velocity", velocity, 1);
#endif

    notelength = new Slider(0, 127, 1, 16, 60, Qt::Horizontal,
            tr("N&ote Length"), this);
    connect(notelength, SIGNAL(valueChanged(int)), this,
            SLOT(updateNoteLength(int)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("NoteLength", notelength, 2);
#endif

    transpose = new Slider(-24, 24, 1, 2, 0, Qt::Horizontal,
            tr("&Transpose"), this);
    connect(transpose, SIGNAL(valueChanged(int)), this,
            SLOT(updateTranspose(int)));
#ifdef APPBUILD
    midiControl->addMidiLearnMenu("Transpose", transpose, 8);
#endif

    QGridLayout* sliderLayout = new QGridLayout;
    sliderLayout->addLayout(dispBoxLayout, 1, 0);
    sliderLayout->addWidget(velocity, 2, 0);
    sliderLayout->addWidget(notelength, 3, 0);
    sliderLayout->addWidget(transpose, 4, 0);
    sliderLayout->setRowStretch(5, 1);
    if (compactStyle) {
        sliderLayout->setSpacing(1);
        sliderLayout->setMargin(2);
    }

    QGridLayout *paramBoxLayout = new QGridLayout;
    paramBoxLayout->addWidget(loopBox, 0, 0, 1, 2);
    paramBoxLayout->addWidget(muteOut, 1, 0, 1, 1);
    paramBoxLayout->addWidget(deferChangesButton, 1, 1, 1, 2);
    paramBoxLayout->addWidget(recordButtonLabel, 2, 0);
    paramBoxLayout->addWidget(recordButton, 2, 1);
    paramBoxLayout->addWidget(resBoxLabel, 3, 0);
    paramBoxLayout->addWidget(resBox, 3, 1);
    paramBoxLayout->addWidget(sizeBoxLabel, 4, 0);
    paramBoxLayout->addWidget(sizeBox, 4, 1);
    paramBoxLayout->setRowStretch(5, 1);

    QGridLayout* seqBoxLayout = new QGridLayout;
    seqBoxLayout->addWidget(screen, 0, 0, 1, 2);
    seqBoxLayout->addWidget(cursor, 1, 0, 1, 2);
    seqBoxLayout->addLayout(paramBoxLayout, 2, 0);
    seqBoxLayout->addLayout(sliderLayout, 2, 1);
    if (compactStyle) {
        seqBoxLayout->setMargin(2);
        seqBoxLayout->setSpacing(0);
    }
    seqBox->setLayout(seqBoxLayout);

    QHBoxLayout *widgetLayout = new QHBoxLayout;
    widgetLayout->addWidget(seqBox, 1);
    widgetLayout->addWidget(hideInOutBoxButton, 0);
    widgetLayout->addWidget(inOutBoxWidget, 0);

    muteOutAction->setChecked(mutedAdd);

    setLayout(widgetLayout);
    recordMode = false;
    updateVelocity(64);
    updateWaveForm(0);
    lastMute = false;
    modified = false;
}

#ifdef APPBUILD
MidiSeq *SeqWidget::getMidiWorker()
{
    return (midiWorker);
}

void SeqWidget::writeData(QXmlStreamWriter& xml)
{
    QByteArray tempArray;
    int l1;

        writeCommonData(xml);

        xml.writeStartElement("display");
            xml.writeTextElement("vertical", QString::number(
                dispVertIndex));
        xml.writeEndElement();

        xml.writeStartElement("seqParams");
            xml.writeTextElement("loopmode", QString::number(
                loopBox->currentIndex()));
            xml.writeTextElement("resolution", QString::number(
                resBox->currentIndex()));
            xml.writeTextElement("size", QString::number(
                sizeBox->currentIndex()));
            xml.writeTextElement("velocity", QString::number(
                midiWorker->vel));
            xml.writeTextElement("noteLength", QString::number(
                tickLenToSlider(midiWorker->notelength)));
            xml.writeTextElement("transp", QString::number(
                midiWorker->transp));
        xml.writeEndElement();

        tempArray.clear();
        l1 = 0;
        while (l1 < midiWorker->maxNPoints) {
            tempArray.append(midiWorker->muteMask.at(l1));
            l1++;
        }
        xml.writeStartElement("muteMask");
            xml.writeTextElement("data", tempArray.toHex());
        xml.writeEndElement();

        tempArray.clear();
        l1 = 0;
        while (l1 < midiWorker->maxNPoints) {
            tempArray.append(midiWorker->customWave.at(l1).value);
            l1++;
        }
        xml.writeStartElement("sequence");
            xml.writeTextElement("data", tempArray.toHex());
            xml.writeTextElement("loopmarker", QString::number(
                getLoopMarker()));
        xml.writeEndElement();

    xml.writeEndElement();
}

void SeqWidget::readData(QXmlStreamReader& xml)
{
    int tmp;
    Sample sample;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement())
            break;
            
        readCommonData(xml);
        
        if (xml.isStartElement() && (xml.name() == "display")) {
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isEndElement())
                    break;
                if (xml.name() == "vertical")
                    setDispVert(xml.readElementText().toInt());
                else skipXmlElement(xml);
            }
        }

        else if (xml.isStartElement() && (xml.name() == "seqParams")) {
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isEndElement())
                    break;
                if (xml.name() == "loopmode") {
                    tmp = xml.readElementText().toInt();
                    loopBox->setCurrentIndex(tmp);
                    updateLoop(tmp);
                }
                else if (xml.name() == "resolution") {
                    tmp = xml.readElementText().toInt();
                    resBox->setCurrentIndex(tmp);
                    updateRes(tmp);
                }
                else if (xml.name() == "size") {
                    tmp = xml.readElementText().toInt();
                    sizeBox->setCurrentIndex(tmp);
                    updateSize(tmp);
                }
                else if (xml.name() == "velocity") {
                    tmp = xml.readElementText().toInt();
                    velocity->setValue(tmp);
                    updateVelocity(tmp);
                }
                else if (xml.name() == "noteLength") {
                    notelength->setValue(xml.readElementText().toInt());
                }
                else if (xml.name() == "transp") {
                    tmp = xml.readElementText().toInt();
                    transpose->setValue(tmp);
                    updateTranspose(tmp);
                }
                else skipXmlElement(xml);
            }
        }
        else if (xml.isStartElement() && (xml.name() == "muteMask")) {
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isEndElement())
                    break;
                if (xml.isStartElement() && (xml.name() == "data")) {
                    QByteArray tmpArray =
                            QByteArray::fromHex(xml.readElementText().toLatin1());
                    for (int l1 = 0; l1 < tmpArray.count(); l1++) {
                        midiWorker->muteMask.replace(l1, tmpArray.at(l1));
                    }
                    midiWorker->maxNPoints = tmpArray.count();
                }
                else skipXmlElement(xml);
            }
        }
        else if (xml.isStartElement() && (xml.name() == "sequence")) {
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isEndElement())
                    break;
                if (xml.isStartElement() && (xml.name() == "data")) {
                    QByteArray tmpArray =
                            QByteArray::fromHex(xml.readElementText().toLatin1());
                    int step = TPQN / midiWorker->res;
                    int lt = 0;
                    for (int l1 = 0; l1 < tmpArray.count(); l1++) {
                        sample.value = tmpArray.at(l1);
                        sample.tick = lt;
                        sample.muted = midiWorker->muteMask.at(l1);
                        midiWorker->customWave.replace(l1, sample);
                        lt+=step;
                    }
                    updateWaveForm(0);
                }
                else if (xml.name() == "loopmarker") {
                    tmp = xml.readElementText().toInt();
                    midiWorker->setLoopMarker(tmp);
                    screen->setLoopMarker(tmp);
                }
                else skipXmlElement(xml);
            }
        }
        else skipXmlElement(xml);
    }
    
    updateChIn(chIn->currentIndex());
    updateChannelOut(channelOut->currentIndex());
    updatePortOut(portOut->currentIndex());
    modified = false;
}
#endif

void SeqWidget::updateNoteLength(int val)
{
    if (midiWorker) midiWorker->updateNoteLength(sliderToTickLen(val));
    modified = true;
}

void SeqWidget::updateWaveForm(int val)
{
    (void)val;
    if (midiWorker) midiWorker->getData(&data);
    screen->updateData(data);
    modified = true;
}

void SeqWidget::setRecord(bool on)
{
    recordMode = on;
    screen->setRecordMode(on);
    if (!midiWorker) return;
    midiWorker->setRecordMode(on);
    screen->setCurrentRecStep(midiWorker->currentRecStep);
}

void SeqWidget::updateRes(int val)
{
    if (val > 4) return;
    resBoxIndex = val;
    if (!midiWorker) return;
    midiWorker->res = seqResValues[val];
    midiWorker->resizeAll();
    midiWorker->getData(&data);
    screen->setCurrentRecStep(midiWorker->currentRecStep);
    screen->updateData(data);
    modified = true;
}

void SeqWidget::updateSize(int val)
{
    if (val > 9) return;
    sizeBoxIndex = val;
    if (!midiWorker) return;
    midiWorker->size = sizeBox->currentText().toInt();
    midiWorker->resizeAll();
    midiWorker->getData(&data);
    screen->setCurrentRecStep(midiWorker->currentRecStep);
    screen->updateData(data);
    modified = true;
}

void SeqWidget::updateLoop(int val)
{
    if (val > 6) return;
    if (midiWorker) midiWorker->updateLoop(val);
    modified = true;
}

void SeqWidget::updateVelocity(int val)
{
    if (midiWorker) midiWorker->updateVelocity(val);
    modified = true;
}

void SeqWidget::updateTranspose(int val)
{
    if (midiWorker) midiWorker->updateTranspose(val);
    modified = true;
}

void SeqWidget::mouseEvent(double mouseX, double mouseY, int buttons, int pressed)
{
    if (!midiWorker) {
        emit mouseSig(mouseX, mouseY, buttons, pressed);
    }
    else {
        midiWorker->mouseEvent(mouseX, mouseY, buttons, pressed);
    }

    if ((mouseY < 0) && (pressed != 2)) { // we have to recalculate loopMarker for screen update
        if (mouseX < 0) mouseX = 0;
        if (buttons == 2) mouseX = - mouseX;
        const int npoints = data.count() - 1;
        int lm;
        if (mouseX > 0) lm = mouseX * (double)npoints + .5;
        else lm = mouseX * (double)npoints - .5;
        if (abs(lm) >= npoints) lm = 0;
        screen->setLoopMarker(lm);
        screen->updateDraw();
    }
    modified = true;
}

void SeqWidget::setMuted(bool on)
{
    if (!midiWorker) return;
    midiWorker->setMuted(on);
    screen->setMuted(midiWorker->isMuted);
#ifdef APPBUILD
    parStore->ndc->setMuted(midiWorker->isMuted);
#endif
    modified = true;
}

void SeqWidget::setDispVert(int mode)
{
    dispVert[mode]->setChecked(true);
}

void SeqWidget::updateDispVert(int mode)
{
    dispVertIndex = mode;
    if (midiWorker) midiWorker->updateDispVert(mode);
    screen->updateDispVert(mode);
    modified = true;
}

#ifdef APPBUILD

void SeqWidget::doStoreParams(int ix, bool empty)
{
    parStore->temp.empty = empty;
    parStore->temp.muteOut = muteOut->isChecked();
    parStore->temp.chIn = chIn->currentIndex();
    parStore->temp.indexIn0 = indexIn[0]->value();
    parStore->temp.indexIn1 = indexIn[1]->value();
    parStore->temp.rangeIn0 = rangeIn[0]->value();
    parStore->temp.rangeIn1 = rangeIn[1]->value();
    parStore->temp.channelOut = channelOut->currentIndex();
    parStore->temp.portOut = portOut->currentIndex();
    parStore->temp.res = resBox->currentIndex();
    parStore->temp.size = sizeBox->currentIndex();
    parStore->temp.loopMode = loopBox->currentIndex();
    parStore->temp.notelen = notelength->value();
    parStore->temp.transp = transpose->value();
    parStore->temp.vel = velocity->value();
    parStore->temp.dispVertIndex = dispVertIndex;
    parStore->temp.loopMode = loopBox->currentIndex();
    parStore->temp.wave = getCustomWave().mid(0, midiWorker->maxNPoints);
    parStore->temp.muteMask = midiWorker->muteMask.mid(0, midiWorker->maxNPoints);
    parStore->temp.loopMarker = getLoopMarker();

    parStore->tempToList(ix);
}

void SeqWidget::doRestoreParams(int ix)
{
    midiWorker->applyPendingParChanges();
    if (parStore->list.at(ix).empty) return;
    for (int l1 = 0; l1 < parStore->list.at(ix).wave.count(); l1++) {
        midiWorker->customWave.replace(l1, parStore->list.at(ix).wave.at(l1));
        midiWorker->muteMask.replace(l1, parStore->list.at(ix).muteMask.at(l1));
    }
    sizeBoxIndex = parStore->list.at(ix).size;
    sizeBox->setCurrentIndex(sizeBoxIndex);
    midiWorker->size = sizeBox->currentText().toInt();
    resBoxIndex = parStore->list.at(ix).res;
    midiWorker->res = seqResValues[resBoxIndex];
    midiWorker->resizeAll();
    midiWorker->setLoopMarker(parStore->list.at(ix).loopMarker);
    screen->setLoopMarker(parStore->list.at(ix).loopMarker);

    resBox->setCurrentIndex(parStore->list.at(ix).res);
    loopBox->setCurrentIndex(parStore->list.at(ix).loopMode);
    if (!parStore->onlyPatternList.at(ix)) {
        midiWorker->notelength = sliderToTickLen(parStore->list.at(ix).notelen);
        midiWorker->transp = parStore->list.at(ix).transp;
        midiWorker->vel = parStore->list.at(ix).vel;
        setDispVert(parStore->list.at(ix).dispVertIndex);

        //muteOut->setChecked(parStore->list.at(ix).muteOut);
        indexIn[0]->setValue(parStore->list.at(ix).indexIn0);
        indexIn[1]->setValue(parStore->list.at(ix).indexIn1);
        rangeIn[0]->setValue(parStore->list.at(ix).rangeIn0);
        rangeIn[1]->setValue(parStore->list.at(ix).rangeIn1);
        chIn->setCurrentIndex(parStore->list.at(ix).chIn);
        updateChIn(parStore->list.at(ix).chIn);
        channelOut->setCurrentIndex(parStore->list.at(ix).channelOut);
        updateChannelOut(parStore->list.at(ix).channelOut);
        setPortOut(parStore->list.at(ix).portOut);
        updatePortOut(parStore->list.at(ix).portOut);
    }
    updateLoop(parStore->list.at(ix).loopMode);
    updateWaveForm(parStore->list.at(ix).waveForm);
    midiWorker->setCurrentIndex(0);

    needsGUIUpdate = true;
}

void SeqWidget::copyParamsFrom(SeqWidget *fromWidget)
{
    int tmp;
    setDispVert(fromWidget->dispVertIndex);
    enableNoteIn->setChecked(fromWidget->enableNoteIn->isChecked());
    enableNoteOff->setChecked(fromWidget->enableNoteOff->isChecked());
    enableVelIn->setChecked(fromWidget->enableVelIn->isChecked());
    enableRestartByKbd->setChecked(fromWidget->enableRestartByKbd->isChecked());
    enableTrigByKbd->setChecked(fromWidget->enableTrigByKbd->isChecked());
    enableTrigLegato->setChecked(fromWidget->enableTrigLegato->isChecked());

    for (int l1 = 0; l1 < 1; l1++) {
        tmp = fromWidget->indexIn[l1]->value();
        indexIn[l1]->setValue(tmp);
    }
    for (int l1 = 0; l1 < 1; l1++) {
        tmp = fromWidget->rangeIn[l1]->value();
        rangeIn[l1]->setValue(tmp);
    }
    tmp = fromWidget->chIn->currentIndex();
    chIn->setCurrentIndex(tmp);
    updateChIn(tmp);
    tmp = fromWidget->channelOut->currentIndex();
    channelOut->setCurrentIndex(tmp);
    updateChannelOut(tmp);
    tmp = fromWidget->portOut->currentIndex();
    portOut->setCurrentIndex(tmp);
    updatePortOut(tmp);

    tmp = fromWidget->resBox->currentIndex();
    resBox->setCurrentIndex(tmp);
    updateRes(tmp);
    tmp = fromWidget->sizeBox->currentIndex();
    sizeBox->setCurrentIndex(tmp);
    updateSize(tmp);
    tmp = fromWidget->loopBox->currentIndex();
    loopBox->setCurrentIndex(tmp);
    updateLoop(tmp);

    tmp = fromWidget->velocity->value();
    updateVelocity(tmp);
    velocity->setValue(tmp);
    tmp = fromWidget->transpose->value();
    updateTranspose(tmp);
    transpose->setValue(tmp);

    notelength->setValue(fromWidget->notelength->value());
    for (int l1 = 0; l1 < fromWidget->getMidiWorker()->maxNPoints; l1++) {
        midiWorker->customWave.replace(l1, fromWidget->getCustomWave().at(l1));
        midiWorker->muteMask.replace(l1, midiWorker->customWave.at(l1).muted);
    }
    tmp = fromWidget->getLoopMarker();
    midiWorker->setLoopMarker(tmp);
    screen->setLoopMarker(tmp);
    midiControl->setCcList(fromWidget->midiControl->ccList);
    muteOutAction->setChecked(true);
    updateWaveForm(0);
}

QVector<Sample> SeqWidget::getCustomWave()
{
    return midiWorker->customWave;
}

void SeqWidget::handleController(int ccnumber, int channel, int value)
{
    bool m;
    int min, max, sval;
    QVector<MidiCC> cclist= midiControl->ccList;

    for (int l2 = 0; l2 < cclist.count(); l2++) {
        min = cclist.at(l2).min;
        max = cclist.at(l2).max;
        if ((ccnumber == cclist.at(l2).ccnumber) &&
            (channel == cclist.at(l2).channel)) {
            switch (cclist.at(l2).ID) {
                case 0: if (min == max) {
                            if (value == max) {
                                m = midiWorker->isMuted;
                                midiWorker->setMuted(!m);
                            }
                        }
                        else {
                            if (value == max) {
                                midiWorker->setMuted(false);
                            }
                            if (value == min) {
                                midiWorker->setMuted(true);
                            }
                        }
                break;

                case 1:
                        sval = min + ((double)value * (max - min) / 127);
                        midiWorker->updateVelocity(sval);
                break;

                case 2:
                        sval = min + ((double)value * (max - min) / 127);
                        midiWorker->updateNoteLength(sliderToTickLen(sval));
                break;

                case 3: if (min == max) {
                            if (value == max) {
                                m = midiWorker->recordMode;
                                midiWorker->setRecordMode(!m);
                                return;
                            }
                        }
                        else {
                            if (value == max) {
                                midiWorker->setRecordMode(true);
                            }
                            if (value == min) {
                                midiWorker->setRecordMode(false);
                            }
                        }
                break;
                case 4:
                        sval = min + ((double)value * (max - min) / 127);
                        if (sval < 5) resBoxIndex = sval;
                break;
                case 5:
                        sval = min + ((double)value * (max - min) / 127);
                        if (sval < 8) sizeBoxIndex = sval;
                break;
                case 6:
                        sval = min + ((double)value * (max - min) / 127);
                        if (sval < 6) midiWorker->curLoopMode = sval;
                break;
                case 7:
                        sval = min + ((double)value * (max - min) / 127);
                        if ((sval < parStore->list.count())
                                && (sval != parStore->activeStore)
                                && (sval != parStore->currentRequest)) {
                            parStore->requestDispState(sval, 2);
                            parStore->restoreRequest = sval;
                            parStore->restoreRunOnce = (parStore->jumpToList.at(sval) > -2);
                        }
                        else return;
                break;

                case 8:
                        sval = min + ((double)value * (max - min) / 127);
                        midiWorker->updateTranspose(sval - 24);
                break;
                
                case 9:
                        sval = min + ((double)value * (max - min) / 127);
                        if (sval < 16) midiWorker->channelOut = sval;
                break;

                default:
                break;
            }
            needsGUIUpdate = true;
        }
    }
}

void SeqWidget::updateDisplay()
{
    QVector<Sample> data;

    parStore->updateDisplay(getCurrentIndex(), midiWorker->reverse);


    if (dataChanged || midiWorker->dataChanged) {
        dataChanged=false;
        midiWorker->dataChanged=false;
        midiWorker->getData(&data);
        screen->updateData(data);
        if (recordMode) screen->setCurrentRecStep(midiWorker->currentRecStep);
        cursor->updateNumbers(midiWorker->res, midiWorker->size);
    }
    screen->updateDraw();
    cursor->updateDraw();
    midiControl->update();

    if (!(needsGUIUpdate || midiWorker->needsGUIUpdate)) return;

    transpose->setValue(midiWorker->transp);
    notelength->setValue(tickLenToSlider(midiWorker->notelength));
    velocity->setValue(midiWorker->vel);
    muteOutAction->setChecked(midiWorker->isMuted);
    screen->setMuted(midiWorker->isMuted);
    parStore->ndc->setMuted(midiWorker->isMuted);
    recordAction->setChecked(midiWorker->recordMode);
    resBox->setCurrentIndex(resBoxIndex);
    updateRes(resBoxIndex);
    sizeBox->setCurrentIndex(sizeBoxIndex);
    updateSize(sizeBoxIndex);
    loopBox->setCurrentIndex(midiWorker->curLoopMode);
    channelOut->setCurrentIndex(midiWorker->channelOut);
    needsGUIUpdate = false;
    midiWorker->needsGUIUpdate = false;
}

#endif
