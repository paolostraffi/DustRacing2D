// This file is part of Dust Racing 2D.
// Copyright (C) 2015 Jussi Lind <jussi.lind@iki.fi>
//
// Dust Racing 2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Dust Racing 2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dust Racing 2D. If not, see <http://www.gnu.org/licenses/>.

#include <QApplication>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QStatusBar>
#include <QString>
#include <QTransform>

#include "editordata.hpp"
#include "editorview.hpp"
#include "mainwindow.hpp"
#include "object.hpp"
#include "objectfactory.hpp"
#include "objectmodelloader.hpp"
#include "rotatedialog.hpp"
#include "targetnode.hpp"
#include "targetnodesizedlg.hpp"
#include "trackdata.hpp"
#include "tracktile.hpp"

#include <cassert>
#include <cstdlib>

EditorView::EditorView(EditorData & editorData, QWidget * parent)
: QGraphicsView(parent)
, m_editorData(editorData)
{
    createTileContextMenuActions();
    createObjectContextMenuActions();
    createTargetNodeContextMenuActions();
}

void EditorView::updateSceneRect()
{
    const QRectF newSceneRect(
        0,
        0,
        m_editorData.trackData()->map().cols() * TrackTile::TILE_W,
        m_editorData.trackData()->map().rows() * TrackTile::TILE_H);

    setSceneRect(newSceneRect);

    assert(scene());
    scene()->setSceneRect(newSceneRect);
}

void EditorView::mouseMoveEvent(QMouseEvent * event)
{
    if (scene())
    {
        const QPointF mappedPos = mapToScene(event->pos());
        if (auto tile = dynamic_cast<TrackTile *>(scene()->itemAt(mappedPos, QTransform())))
        {
            tile->setActive(true);
        }

        // Tile drag'n'drop active?
        if (auto sourceTile = m_editorData.dragAndDropSourceTile())
        {
            sourceTile->setPos(mappedPos);
        }
        // Object drag'n'drop active?
        else if (auto object = m_editorData.dragAndDropObject())
        {
            object->setLocation(mappedPos);
        }
        // Target node drag'n'drop active?
        else if (auto tnode = m_editorData.dragAndDropTargetNode())
        {
            tnode->setLocation(mappedPos);
        }

        updateCoordinates(mappedPos);
    }
}

void EditorView::updateCoordinates(QPointF mappedPos)
{
    if (m_editorData.trackData())
    {
        const int maxCols = static_cast<int>(m_editorData.trackData()->map().cols());
        int column = mappedPos.x() / TrackTile::TILE_W;
        column = column >= maxCols ? maxCols - 1 : column;
        column = column < 0 ? 0 : column;

        const int maxRows = static_cast<int>(m_editorData.trackData()->map().rows());
        int row = mappedPos.y() / TrackTile::TILE_H;
        row = row >= maxRows ? maxRows - 1 : row;
        row = row < 0 ? 0 : row;

        m_editorData.setActiveRow(static_cast<unsigned int>(row));
        m_editorData.setActiveColumn(static_cast<unsigned int>(column));

        QString coordinates("X: %1 Y: %2 I: %3 J: %4");
        coordinates = coordinates.arg(mappedPos.x()).arg(mappedPos.y()).arg(column).arg(row);
        MainWindow::instance()->statusBar()->showMessage(coordinates);
    }
}

void EditorView::createTileContextMenuActions()
{
    const QChar degreeSign(176);
    const QString dummy1(QString(QWidget::tr("Rotate 90")) + degreeSign + QWidget::tr(" CW.."));

    auto rotate90CW = new QAction(dummy1, &m_tileContextMenu);
    QObject::connect(rotate90CW, &QAction::triggered, [this] () {
        if (TrackTile * tile =
            dynamic_cast<TrackTile *>(scene()->itemAt(mapToScene(m_clickedPos), QTransform())))
        {
            m_editorData.saveUndoPoint();
            tile->rotate90CW();
        }
    });

    const QString dummy2(QString(QWidget::tr("Rotate 90")) + degreeSign + QWidget::tr(" CCW.."));

    auto rotate90CCW = new QAction(dummy2, &m_tileContextMenu);
    QObject::connect(rotate90CCW, &QAction::triggered, [this] () {
        if (auto tile =
            dynamic_cast<TrackTile *>(scene()->itemAt(mapToScene(m_clickedPos), QTransform())))
        {
            m_editorData.saveUndoPoint();
            tile->rotate90CCW();
        }
    });

    m_clearComputerHint = new QAction(QWidget::tr("Clear computer hint"), &m_tileContextMenu);
    QObject::connect(m_clearComputerHint, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        setComputerHint(TrackTileBase::CH_NONE);
    });

    m_setComputerHintBrakeHard = new QAction(
        QWidget::tr("Set computer hint 'brake hard'.."), &m_tileContextMenu);
    QObject::connect(m_setComputerHintBrakeHard, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        setComputerHint(TrackTileBase::CH_BRAKE_HARD);
    });

    m_setComputerHintBrake = new QAction(
        QWidget::tr("Set computer hint 'brake'.."), &m_tileContextMenu);
    QObject::connect(m_setComputerHintBrake, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        setComputerHint(TrackTileBase::CH_BRAKE);
    });

    m_excludeFromMinimap = new QAction(
        QWidget::tr("Exclude from minimap"), &m_tileContextMenu);
    m_excludeFromMinimap->setCheckable(true);
    QObject::connect(m_excludeFromMinimap, &QAction::changed, [this] () {
        if (auto tile = dynamic_cast<TrackTile *>(scene()->itemAt(mapToScene(this->m_clickedPos), QTransform())))
        {
            m_editorData.saveUndoPoint();
            tile->setExcludeFromMinimap(this->m_excludeFromMinimap->isChecked());
        }
    });

    auto insertRowBefore = new QAction(
        QWidget::tr("Insert row before.."), &m_tileContextMenu);
    QObject::connect(insertRowBefore, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        m_editorData.trackData()->insertRow(m_editorData.activeRow(), MapBase::InsertDirection::Before);
        m_editorData.addTilesToScene();
        updateSceneRect();
        update();
    });

    auto insertRowAfter = new QAction(
        QWidget::tr("Insert row after.."), &m_tileContextMenu);
    QObject::connect(insertRowAfter, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        m_editorData.trackData()->insertRow(m_editorData.activeRow(), MapBase::InsertDirection::After);
        m_editorData.addTilesToScene();
        updateSceneRect();
        update();
    });

    m_deleteRow = new QAction(
        QWidget::tr("Delete row.."), &m_tileContextMenu);
    QObject::connect(m_deleteRow, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        auto deleted = m_editorData.trackData()->deleteRow(m_editorData.activeRow());
        for (auto && tile : deleted) {
            m_editorData.removeTileFromScene(tile);
        }
        updateSceneRect();
        update();
    });

    auto insertColBefore = new QAction(
        QWidget::tr("Insert column before.."), &m_tileContextMenu);
    QObject::connect(insertColBefore, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        m_editorData.trackData()->insertColumn(m_editorData.activeColumn(), MapBase::InsertDirection::Before);
        m_editorData.addTilesToScene();
        updateSceneRect();
        update();
    });

    auto insertColAfter = new QAction(
        QWidget::tr("Insert column after.."), &m_tileContextMenu);
    QObject::connect(insertColAfter, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        m_editorData.trackData()->insertColumn(m_editorData.activeColumn(), MapBase::InsertDirection::After);
        m_editorData.addTilesToScene();
        updateSceneRect();
        update();
    });

    m_deleteCol = new QAction(
        QWidget::tr("Delete column.."), &m_tileContextMenu);
    QObject::connect(m_deleteCol, &QAction::triggered, [this] () {
        m_editorData.saveUndoPoint();
        auto deleted = m_editorData.trackData()->deleteColumn(m_editorData.activeColumn());
        for (auto && tile : deleted) {
            m_editorData.removeTileFromScene(tile);
        }
        updateSceneRect();
        update();
    });

    // Populate the menu
    m_tileContextMenu.addAction(rotate90CW);
    m_tileContextMenu.addAction(rotate90CCW);
    m_tileContextMenu.addSeparator();
    m_tileContextMenu.addAction(m_clearComputerHint);
    m_tileContextMenu.addAction(m_setComputerHintBrakeHard);
    m_tileContextMenu.addAction(m_setComputerHintBrake);
    m_tileContextMenu.addSeparator();
    m_tileContextMenu.addAction(m_excludeFromMinimap);
    m_tileContextMenu.addSeparator();
    m_tileContextMenu.addAction(insertRowBefore);
    m_tileContextMenu.addAction(insertRowAfter);
    m_tileContextMenu.addAction(m_deleteRow);
    m_tileContextMenu.addSeparator();
    m_tileContextMenu.addAction(insertColBefore);
    m_tileContextMenu.addAction(insertColAfter);
    m_tileContextMenu.addAction(m_deleteCol);
}

void EditorView::createObjectContextMenuActions()
{
    const QString dummy1(QString(QWidget::tr("Rotate..")));
    auto rotate = new QAction(dummy1, &m_tileContextMenu);
    QObject::connect(rotate, &QAction::triggered, [this] () {
        RotateDialog dialog;
        if (dialog.exec() == QDialog::Accepted)
        {
            if (auto object = m_editorData.selectedObject())
            {
                m_editorData.saveUndoPoint();

                object->setRotation(static_cast<int>(dialog.angle() + object->rotation()) % 360);
            }
        }
    });

    const QString dummy2(QString(QWidget::tr("Force stationary")));
    m_forceStationaryAction = new QAction(dummy2, &m_tileContextMenu);
    m_forceStationaryAction->setCheckable(true);
    QObject::connect(m_forceStationaryAction, &QAction::changed, [this] () {
        if (auto object = m_editorData.selectedObject())
        {
            m_editorData.saveUndoPoint();
            object->setForceStationary(m_forceStationaryAction->isChecked());
        }
    });

    // Populate the menu
    m_objectContextMenu.addAction(rotate);
    m_objectContextMenu.addSeparator();
    m_objectContextMenu.addAction(m_forceStationaryAction);
}

void EditorView::createTargetNodeContextMenuActions()
{
    const QString dummy1(QString(QWidget::tr("Set size..")));
    auto setSize = new QAction(dummy1, &m_targetNodeContextMenu);
    QObject::connect(setSize, &QAction::triggered, [this] () {
        if (auto tnode = m_editorData.selectedTargetNode())
        {
            TargetNodeSizeDlg dialog(tnode->size());
            if (dialog.exec() == QDialog::Accepted)
            {
                m_editorData.saveUndoPoint();

                tnode->setSize(dialog.targetNodeSize());
            }
        }
    });

    // Populate the menu
    m_targetNodeContextMenu.addAction(setSize);
}

void EditorView::mousePressEvent(QMouseEvent * event)
{
    if (scene())
    {
        m_clickedPos      = event->pos();
        m_clickedScenePos = mapToScene(m_clickedPos);

        // Fetch all items at the location
        QList<QGraphicsItem *> items = scene()->items(
                    m_clickedScenePos, Qt::IntersectsItemShape, Qt::DescendingOrder);

        if (items.size())
        {
            // Check if there's an object at the position and handle that.
            // Otherwise it would be difficult to select objects that are surrounded
            // by a target node area.
            auto iter = items.begin();
            while (iter != items.end())
            {
                auto item = *iter;
                if (auto object = dynamic_cast<Object *>(item))
                {
                    handleMousePressEventOnObject(*event, *object);
                    return;
                }

                iter++;
            }

            auto item = *items.begin();
            if (auto tnode = dynamic_cast<TargetNode *>(item))
            {
                handleMousePressEventOnTargetNode(*event, *tnode);
            }
            else if (auto tile = dynamic_cast<TrackTile *>(item))
            {
                handleMousePressEventOnTile(*event, *tile);
            }
        }
    }
}

void EditorView::eraseObjectAtCurrentClickedPos()
{
    m_editorData.saveUndoPoint();

    // Fetch all items at the location
    QList<QGraphicsItem *> items = scene()->items(
        m_clickedScenePos, Qt::IntersectsItemShape, Qt::DescendingOrder);

    // We need to find the first object to be erased, because
    // there might be also overlapping TargetNodes.
    for (auto item : items)
    {
        if (auto object = dynamic_cast<Object *>(item))
        {
            scene()->removeItem(object);
            m_editorData.trackData()->objects().remove(*object);
            m_editorData.setSelectedObject(nullptr);
            break;
        }
    }
}

void EditorView::addCurrentToolBarObjectToScene()
{
    if (QAction * action = MainWindow::instance()->currentToolBarAction())
    {
        if (scene())
        {
            m_editorData.saveUndoPoint();

            Object & object = ObjectFactory::createObject(action->data().toString());
            object.setLocation(m_clickedScenePos);

            if (MainWindow::instance()->randomlyRotateObjects())
            {
                object.setRotation(std::rand() % 360);
            }

            scene()->addItem(&object);

            m_editorData.trackData()->objects().add(ObjectBasePtr(&object));
            m_editorData.setSelectedObject(&object);
        }
    }
}

void EditorView::handleMousePressEventOnObject(QMouseEvent & event, Object & object)
{
    if (event.button() == Qt::RightButton)
    {
        handleRightButtonClickOnObject(object);
    }
    else if (event.button() == Qt::LeftButton)
    {
        // User is adding an object
        if (m_editorData.mode() == EditorData::EditorMode::AddObject)
        {
            addCurrentToolBarObjectToScene();
        }
        // User is erasing an object
        else if (m_editorData.mode() == EditorData::EditorMode::EraseObject)
        {
            eraseObjectAtCurrentClickedPos();
        }
        // Default actions
        else
        {
            handleLeftButtonClickOnObject(object);
        }
    }

    QWidget::mousePressEvent(&event);
}

void EditorView::handleMousePressEventOnTargetNode(QMouseEvent & event, TargetNode & tnode)
{
    if (event.button() == Qt::RightButton)
    {
        handleRightButtonClickOnTargetNode(tnode);
    }
    else if (event.button() == Qt::LeftButton)
    {
        // User is adding an object
        if (m_editorData.mode() == EditorData::EditorMode::AddObject)
        {
            addCurrentToolBarObjectToScene();
        }
        else
        {
            handleLeftButtonClickOnTargetNode(tnode);
        }
    }

    QWidget::mousePressEvent(&event);
}

void EditorView::handleMousePressEventOnTile(QMouseEvent & event, TrackTile & tile)
{
    tile.setActive(true);

    if (event.button() == Qt::RightButton)
    {
        handleRightButtonClickOnTile(tile);
    }
    else if (event.button() == Qt::LeftButton)
    {
        // User is adding an object
        if (m_editorData.mode() == EditorData::EditorMode::AddObject)
        {
            addCurrentToolBarObjectToScene();
        }
        else
        {
            handleLeftButtonClickOnTile(tile);
        }
    }

    QWidget::mousePressEvent(&event);
}

void EditorView::handleLeftButtonClickOnObject(Object & object)
{
    // User is initiating a drag'n'drop
    if (m_editorData.mode() == EditorData::EditorMode::None)
    {
        m_editorData.saveUndoPoint();

        object.setZValue(object.zValue() + 1);
        m_editorData.setDragAndDropObject(&object);
        m_editorData.setSelectedObject(&object);

        // Change cursor to the closed hand cursor.
        QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
    }
}

void EditorView::handleLeftButtonClickOnTargetNode(TargetNode & tnode)
{
    // User is initiating a drag'n'drop
    if (m_editorData.mode() == EditorData::EditorMode::None)
    {
        m_editorData.saveUndoPoint();

        tnode.setZValue(tnode.zValue() + 1);
        m_editorData.setDragAndDropTargetNode(&tnode);

        // Change cursor to the closed hand cursor.
        QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
    }
    // It's not possible to make nodes overlap if not handled also here.
    else if (m_editorData.mode() == EditorData::EditorMode::SetRoute)
    {
        m_editorData.pushNewTargetNodeToRoute(m_clickedScenePos);
    }
}

void EditorView::handleLeftButtonClickOnTile(TrackTile & tile)
{
    // User is defining the route
    if (m_editorData.mode() == EditorData::EditorMode::SetRoute)
    {
        m_editorData.pushNewTargetNodeToRoute(m_clickedScenePos);
    }
    // User is setting the tile type
    else if (m_editorData.mode() == EditorData::EditorMode::SetTileType)
    {
        if (QAction * action = MainWindow::instance()->currentToolBarAction())
        {
            Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

            if (modifiers & Qt::ControlModifier)
            {
                QString typeToFill = tile.tileType();

                if (typeToFill != action->data().toString())
                {
                    doFloodFill(tile, action, typeToFill);
                }
            }
            else
            {
                changeTileType(tile, action);
            }
        }
    }
    // User is initiating a drag'n'drop
    else if (m_editorData.mode() == EditorData::EditorMode::None)
    {
        tile.setZValue(tile.zValue() + 1);
        m_editorData.setDragAndDropSourceTile(&tile);
        m_editorData.setDragAndDropSourcePos(tile.pos());

        // Change cursor to the closed hand cursor.
        QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
    }
}

void EditorView::openTileContextMenu(TrackTile & tile)
{
    // Enable all hint actions by default
    m_clearComputerHint->setEnabled(true);
    m_setComputerHintBrakeHard->setEnabled(true);
    m_setComputerHintBrake->setEnabled(true);

    switch (tile.computerHint())
    {
    case TrackTileBase::CH_NONE:
        m_clearComputerHint->setEnabled(false);
        break;
    case TrackTileBase::CH_BRAKE_HARD:
        m_setComputerHintBrakeHard->setEnabled(false);
        break;
    case TrackTileBase::CH_BRAKE:
        m_setComputerHintBrake->setEnabled(false);
        break;
    default:
        break;
    }

    m_excludeFromMinimap->setChecked(tile.excludeFromMinimap());

    if (m_editorData.trackData())
    {
        m_deleteCol->setEnabled(m_editorData.trackData()->map().cols() > 2);
        m_deleteRow->setEnabled(m_editorData.trackData()->map().rows() > 2);
    }

    const QPoint globalPos = mapToGlobal(m_clickedPos);
    m_tileContextMenu.exec(globalPos);
}

void EditorView::handleRightButtonClickOnTile(TrackTile & tile)
{
    openTileContextMenu(tile);
}

void EditorView::openObjectContextMenu()
{
    const QPoint globalPos = mapToGlobal(m_clickedPos);
    m_objectContextMenu.exec(globalPos);
}

void EditorView::handleRightButtonClickOnObject(Object & object)
{
    m_editorData.setSelectedObject(&object);

    m_forceStationaryAction->setChecked(object.forceStationary());

    openObjectContextMenu();
}

void EditorView::openTargetNodeContextMenu()
{
    const QPoint globalPos = mapToGlobal(m_clickedPos);
    m_targetNodeContextMenu.exec(globalPos);
}

void EditorView::handleRightButtonClickOnTargetNode(TargetNode & tnode)
{
    m_editorData.setSelectedTargetNode(&tnode);

    openTargetNodeContextMenu();
}

void EditorView::mouseReleaseEvent(QMouseEvent * event)
{
    handleTileDragRelease(event);
    handleObjectDragRelease(event);
    handleTargetNodeDragRelease(event);
}

void EditorView::keyPressEvent(QKeyEvent * event)
{
    if (auto object = m_editorData.selectedObject())
    {
        if (!event->isAutoRepeat())
        {
            switch (event->key())
            {
            case Qt::Key_Left:
                m_editorData.saveUndoPoint();
                object->setLocation(QPointF(object->location().x() - 1, object->location().y()));
                break;
            case Qt::Key_Right:
                m_editorData.saveUndoPoint();
                object->setLocation(QPointF(object->location().x() + 1, object->location().y()));
                break;
            case Qt::Key_Up:
                m_editorData.saveUndoPoint();
                object->setLocation(QPointF(object->location().x(), object->location().y() - 1));
                break;
            case Qt::Key_Down:
                m_editorData.saveUndoPoint();
                object->setLocation(QPointF(object->location().x(), object->location().y() + 1));
                break;
            default:
                break;
            }
        }
    }
}

void EditorView::handleTileDragRelease(QMouseEvent * event)
{
    if (scene())
    {
        // Tile drag'n'drop active?
        if (auto sourceTile = m_editorData.dragAndDropSourceTile())
        {
            // Determine the dest tile
            auto destTile = sourceTile;
            QList<QGraphicsItem *> items = scene()->items(mapToScene(event->pos()));
            for (QGraphicsItem * item : items)
            {
                auto testTile = dynamic_cast<TrackTile *>(item);
                if (testTile && testTile != sourceTile)
                {
                    destTile = testTile;
                    break;
                }
            }

            // Swap tiles
            sourceTile->swap(*destTile);

            // Restore position
            sourceTile->setPos(m_editorData.dragAndDropSourcePos());
            sourceTile->setZValue(sourceTile->zValue() - 1);
            destTile->setZValue(sourceTile->zValue());

            update();

            m_editorData.setDragAndDropSourceTile(nullptr);

            // Restore the cursor.
            QApplication::restoreOverrideCursor();
        }
    }
}

void EditorView::handleObjectDragRelease(QMouseEvent * event)
{
    if (scene())
    {
        // Object drag'n'drop active?
        if (auto object = m_editorData.dragAndDropObject())
        {
            // Set the new position position
            object->setLocation(mapToScene(event->pos()));
            object->setZValue(object->zValue() - 1);

            update();

            m_editorData.setDragAndDropObject(nullptr);

            // Restore the cursor.
            QApplication::restoreOverrideCursor();
        }
    }
}

void EditorView::handleTargetNodeDragRelease(QMouseEvent * event)
{
    if (scene())
    {
        // Target node drag'n'drop active?
        if (auto tnode = m_editorData.dragAndDropTargetNode())
        {
            // Set the new position position
            tnode->setLocation(mapToScene(event->pos()));
            tnode->setZValue(tnode->zValue() - 1);

            update();

            m_editorData.setDragAndDropTargetNode(nullptr);

            // Restore the cursor.
            QApplication::restoreOverrideCursor();
        }
    }
}

void EditorView::setComputerHint(TrackTileBase::ComputerHint hint)
{
    if (auto tile = dynamic_cast<TrackTile *>(scene()->itemAt(mapToScene(m_clickedPos), QTransform())))
    {
        m_editorData.saveUndoPoint();

        tile->setComputerHint(hint);
    }
}

void EditorView::doFloodFill(TrackTile & tile, QAction * action, QString typeToFill)
{
    m_editorData.saveUndoPoint();

    floodFill(tile, action, typeToFill);
}

void EditorView::floodFill(TrackTile & tile, QAction * action, const QString & typeToFill)
{
    static const int DIRECTION_COUNT = 4;

    // Coordinates of neighbor tiles can be calculated by adding these
    // adjustments to tile coordinates.
    static const QPoint neighborAdjustments[DIRECTION_COUNT] =
    {
        QPoint( 1,  0),  // right
        QPoint( 0, -1),  // up
        QPoint(-1,  0),  // left
        QPoint( 0,  1)   // down
    };

    setTileType(tile, action);

    MapBase & map = m_editorData.trackData()->map();
    QPoint location = tile.matrixLocation();

    for (int i = 0; i < DIRECTION_COUNT; ++i)
    {
        int x = location.x() + neighborAdjustments[i].x();
        int y = location.y() + neighborAdjustments[i].y();

        if (x >= 0 && y >= 0)
        {
            auto tile = std::dynamic_pointer_cast<TrackTile>(map.getTile(x, y));
            if (tile && tile->tileType() == typeToFill)
            {
                floodFill(*(tile.get()), action, typeToFill);
            }
        }
    }
}

void EditorView::changeTileType(TrackTile & tile, QAction * action)
{
    m_editorData.saveUndoPoint();

    setTileType(tile, action);
}

void EditorView::setTileType(TrackTile & tile, QAction * action)
{
    tile.setTileType(action->data().toString());
    tile.setPixmap(action->icon().pixmap(TrackTile::TILE_W, TrackTile::TILE_H));
}
