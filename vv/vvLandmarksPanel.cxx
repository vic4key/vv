#ifndef _vvLandmarksPanel_CXX
#define _vvLandmarksPanel_CXX

/*=========================================================================

 Program:   vv
 Language:  C++
 Author :   Pierre Seroul (pierre.seroul@gmail.com)

Copyright (C) 200COLUMN_IMAGE_NAME
Léon Bérard cancer center http://oncora1.lyon.fnclcc.fr
CREATIS-LRMN http://www.creatis.insa-lyon.fr

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#include "vvLandmarksPanel.h"

#include <QtGui>
#include <Qt>
#include "QTreePushButton.h"
#include "vvLandmarks.h"

#include <vtksys/SystemTools.hxx>

//====================================================================
vvLandmarksPanel::vvLandmarksPanel(QWidget * parent):QWidget(parent)
{
    setupUi(this);

    tableWidget->verticalHeader()->hide();
    loadButton->setEnabled(0);
    saveButton->setEnabled(0);
    removeButton->setEnabled(0);
    connect(loadButton, SIGNAL(clicked()),this,SLOT(Load()));
    connect(saveButton, SIGNAL(clicked()),this,SLOT(Save()));
    connect(removeButton, SIGNAL(clicked()),this,SLOT(RemoveLastPoint()));
    connect(tableWidget,SIGNAL(cellChanged(int,int)),this,SLOT(CommentsChanged(int,int)));
}

void vvLandmarksPanel::Load()
{
    QString file = QFileDialog::getOpenFileName(this,tr("Load Landmarks"),
                   mCurrentPath.c_str(),tr("Landmarks ( *.txt)"));
    if (!file.isEmpty())
        mCurrentLandmarks->LoadFile(file.toStdString());
    SetCurrentLandmarks(mCurrentLandmarks,2);
    emit UpdateRenderWindows();
}

void vvLandmarksPanel::Save()
{
    QString file = QFileDialog::getSaveFileName(this,
                   tr("Save Landmarks"),
                   mCurrentPath.c_str(),tr("Landmarks ( *.txt)"));
    if (!file.isEmpty())
    {
        std::string filename = vtksys::SystemTools::GetFilenamePath(file.toStdString());
        filename += "/" + vtksys::SystemTools::GetFilenameWithoutLastExtension(file.toStdString());
        filename += ".txt";
        mCurrentLandmarks->SaveFile(filename.c_str());
    }
}

void vvLandmarksPanel::RemoveLastPoint()
{
    if (tableWidget->rowCount() > 0)
    {
        tableWidget->removeRow(tableWidget->rowCount()-1);
        mCurrentLandmarks->RemoveLastLandmark();
        emit UpdateRenderWindows();
    }
}

void vvLandmarksPanel::AddPoint()
{
    AddPoint(mCurrentLandmarks->GetNumberOfPoints()-1);
}

void vvLandmarksPanel::AddPoint(int landmarksIndex)
{
    int rowIndex = landmarksIndex; //tableWidget->rowCount();
    tableWidget->setRowCount(rowIndex+1);
    tableWidget->setRowHeight(rowIndex,20);
    QTableWidgetItem* iItem = new QTableWidgetItem(QString::number(landmarksIndex));
    iItem->setFlags(!Qt::ItemIsEditable);
    tableWidget->setItem(rowIndex,0,iItem);

    QTableWidgetItem* xItem = new QTableWidgetItem(
        QString::number(mCurrentLandmarks->GetCoordinates(landmarksIndex)[0],'f',1));
    xItem->setFlags(!Qt::ItemIsEditable);
    tableWidget->setItem(rowIndex,1,xItem);

    QTableWidgetItem* yItem = new QTableWidgetItem(
        QString::number(mCurrentLandmarks->GetCoordinates(landmarksIndex)[1],'f',1));
    yItem->setFlags(!Qt::ItemIsEditable);
    tableWidget->setItem(rowIndex,2,yItem);

    QTableWidgetItem* zItem = new QTableWidgetItem(
        QString::number(mCurrentLandmarks->GetCoordinates(landmarksIndex)[2],'f',1));
    zItem->setFlags(!Qt::ItemIsEditable);
    tableWidget->setItem(rowIndex,3,zItem);

    QTableWidgetItem* tItem = new QTableWidgetItem(
        QString::number(mCurrentLandmarks->GetCoordinates(landmarksIndex)[3],'f',1));
    tItem->setFlags(!Qt::ItemIsEditable);
    tableWidget->setItem(rowIndex,4,tItem);


    QTableWidgetItem* vItem = new QTableWidgetItem(
        QString::number(mCurrentLandmarks->GetPixelValue(landmarksIndex),'f',1));
    vItem->setFlags(!Qt::ItemIsEditable);
    tableWidget->setItem(rowIndex,5,vItem);

    tableWidget->setItem(rowIndex,6, new QTableWidgetItem(mCurrentLandmarks->GetComments(landmarksIndex).c_str()));
}

void vvLandmarksPanel::SetCurrentLandmarks(vvLandmarks* lm,int time)
{
    loadButton->setEnabled(1);
    saveButton->setEnabled(1);
    removeButton->setEnabled(1);
    mCurrentLandmarks = lm;
    tableWidget->clearContents();
    tableWidget->setRowCount(mCurrentLandmarks->GetNumberOfPoints());
    for (int i = 0; i < mCurrentLandmarks->GetNumberOfPoints(); i++)
        AddPoint(i);
    //if (time > 1)
        //tableWidget->setColumnHidden(4,1);
    //else
        //tableWidget->setColumnHidden(4,0);
    tableWidget->resizeColumnsToContents();
}

void vvLandmarksPanel::SetCurrentImage(std::string filename)
{
    QString image = "<b>CurrentImage : </b>";
    image += vtksys::SystemTools::GetFilenameWithoutLastExtension(filename).c_str();
    nameLabel->setText(image);
}

void vvLandmarksPanel::CommentsChanged(int row, int column)
{
    if (column == 6)
    {
        mCurrentLandmarks->ChangeComments(row,std::string(tableWidget->item(row,column)->text().toStdString()));
        tableWidget->resizeColumnsToContents();
    }
}

#endif /* end #define _vvLandmarksPanel_CXX */

