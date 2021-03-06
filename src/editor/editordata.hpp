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

#ifndef EDITORDATA_HPP
#define EDITORDATA_HPP

#include <memory>
#include <vector>

#include <QString>
#include <QPointF>

#include "trackdata.hpp"
#include "trackio.hpp"
#include "undostack.hpp"

class MainWindow;
class Object;
class ObjectModelLoader;
class TargetNode;
class TargetNodeBase;
class TrackTile;
class QGraphicsLineItem;

//! Editor data includes data and functionality related to the current
//! race track and editing session.
class EditorData
{
public:

    //! Editing modes.
    enum class EditorMode
    {
        None,
        SetRoute,
        SetTileType,
        AddObject,
        EraseObject
    };

    //! Constructor.
    //! \param mainWindow Pointer to the main window.
    EditorData(MainWindow * mainWindow);

    //! Load track from the given file.
    bool loadTrackData(QString fileName);

    bool isUndoable() const;

    //! Load track from undo point.
    void undo();

    bool isRedoable() const;

    //! Load track from redo point.
    void redo();

    //! Save track data.
    bool saveTrackData();

    //! Sava track data to the given file.
    bool saveTrackDataAs(QString fileName);

    //! Save undo point.
    void saveUndoPoint();

    //! Save redo point.
    void saveRedoPoint();

    //! Set track data as the given data.
    void setTrackData(TrackDataPtr newTrackData);

    //! Check if it's possible to set route.
    bool canRouteBeSet() const;

    //! This should be called before user starts
    //! setting the route.
    void beginSetRoute();

    //! This should be called after user ends
    //! setting the route.
    void endSetRoute();

    //! Adds current target node objects to the scene.
    void addExistingRouteToScene();

    //! Push a new target node.
    void pushNewTargetNodeToRoute(QPointF pos);

    //! Removes target node and line objects from the scene.
    void removeRouteFromScene();

    //! Removes given tile from the scene.
    void removeTileFromScene(TrackTileBasePtr trackTile);

    //! Returns current track data object. Returns NULL if not set.
    TrackDataPtr trackData();

    //! Returns current editing mode.
    EditorMode mode() const;

    //! Sets the current editing mode.
    void setMode(EditorMode newMode);

    //! Set tile that is being drag'n'dropped.
    void setDragAndDropSourceTile(TrackTile * tile);

    //! Returns the tile being drag'n'dropped or nullptr.
    TrackTile * dragAndDropSourceTile() const;

    //! Set object that is being drag'n'dropped.
    void setDragAndDropObject(Object * object);

    //! Returns the object being drag'n'dropped or nullptr.
    Object * dragAndDropObject() const;

    //! Set selected object.
    void setSelectedObject(Object * object);

    //! Returns the selected object or nullptr.
    Object * selectedObject() const;

    //! Set selected target node.
    void setSelectedTargetNode(TargetNode * tnode);

    //! Returns the selected target node or nullptr.
    TargetNode * selectedTargetNode() const;

    //! Set target node that is being drag'n'dropped.
    void setDragAndDropTargetNode(TargetNode * tnode);

    //! Returns the target node being drag'n'dropped or nullptr.
    TargetNode * dragAndDropTargetNode() const;

    //! Get source pos of the drag'n'drop.
    QPointF dragAndDropSourcePos() const;

    //! Set source pos of the drag'n'drop.
    void setDragAndDropSourcePos(QPointF pos);

    //! Add tiles in current track data object to the scene.
    void addTilesToScene();

    //! Add objects in current track data object to the scene.
    void addObjectsToScene();

    //! Clear the current route only.
    void clearRoute();

    void setActiveColumn(unsigned int column);

    unsigned int activeColumn() const;

    void setActiveRow(unsigned int row);

    unsigned int activeRow() const;

private:

    EditorData(const EditorData & e);
    EditorData & operator= (const EditorData & e);

    void clearScene();

    void pushTargetNodeToRoute(TargetNodeBasePtr tnode);

    void removeTilesFromScene();

    void removeObjectsFromScene();

    void removeTargetNodesFromScene();

    TrackIO m_trackIO;

    TrackDataPtr m_trackData;

    UndoStack m_undoStack;

    EditorMode m_mode;

    TrackTile * m_dragAndDropSourceTile = nullptr;

    Object * m_dragAndDropObject = nullptr;

    Object * m_selectedObject = nullptr;

    TargetNode * m_selectedTargetNode = nullptr;

    TargetNode * m_dragAndDropTargetNode = nullptr;

    QPointF m_dragAndDropSourcePos;

    MainWindow * m_mainWindow = nullptr;

    std::vector<QGraphicsLineItem *> m_targetNodes;

    unsigned int m_activeColumn = 0;

    unsigned int m_activeRow = 0;
};

#endif // EDITORDATA_HPP
