MainMenu = {
    {File
        {"About..." About()}
        {"Save layout" Save(Layout)}
        {"Save layout as..." Save(LayoutAs)}
        {"Load layout" Load(Layout)}
        {"Load element data to paste-buffer" PasteBuffer(Clear) Load(ElementTobuffer)}
        {"Load layout data to paste-buffer" PasteBuffer(Clear) Load(LayoutTobuffer)}
        {"Load netlist file" Load(Netlist)}
        {"Print layout..." Print() ActiveWhen(DataNonEmpty)}
        -
        {"Save connection data of..." foreground=red sensitive=false}
        {" a single element" GetXY(press a button at the element's location) Save(ElementConnections)}
        {" all elements" Save(AllConnections)}
        {" unused pins" Save(AllUnusedPins)}
        -
        {"Start new layout" New()}
        -
        {"Quit Program" Quit()}
        }
    
    {Edit
        {"Undo last operation" Undo()}
        {"Redo last undone operation" Redo()}
        {"Clear undo-buffer" Undo(ClearList)}
        -
        {"Cut selection to buffer" GetXY(Press a button at the element's location)
            PasteBuffer(Clear) PasteBuffer(AddSelected) RemoveSelected() Mode(PasteBuffer)}
        {"Copy selection to buffer" GetXY(Press a button at the element's location)
            PasteBuffer(Clear) PasteBuffer(AddSelected) Mode(PasteBuffer)}
        {"Paste buffer to layout" Mode(PasteBuffer)}
        -
        {"Unselect all" Unselect(All)}
        {"Select all" Select(All)}
        -
        {"Edit Names..." foreground=red sensitive=false}
        {" Change text on layout" ChangeName(Object)}
        {" Edit name of layout" ChangeName(Layout)}
        {" Edit name of active layer" ChangeName(Layer)}
        -
        {"Change board size..." AdjustStyle(0)}
        }
    
    {Screen
        {"Redraw layout" Display(Redraw)}
        {"Center layout" Display(Center)}
        -
        {"Display grid" CheckWhen(drawgrid) ActiveWhen(gridfactor) Display(Grid)}
        {"Realign grid" GetXY(Press a button at a grid point) Display(ToggleGrid)}
        {"View solder side" CheckWhen(showsolderside) GetXY(Press a button at the desired point) SwapSides()}
        {"Show soldermask" CheckWhen(showmask) Display(ToggleMask)}
        -
        {"Grid Setting..." foreground=red sensitive=false vertSpace=60}
        {"0.1 mil" SetValue(Grid,10) CheckWhen(grid,10)}
        {"0.5 mil" SetValue(Grid,50) CheckWhen(grid,50)}
        {"1 mil" SetValue(Grid,100) CheckWhen(grid,100)}
        {"0.1 mm" SetValue(Grid,393.7007874) CheckWhen(grid,393)}
        {"10 mil" SetValue(Grid,1000) CheckWhen(grid,1000)}
        {"1 mm" SetValue(Grid,3937.007874) CheckWhen(grid,3937)}
        {"25 mil" SetValue(Grid,2500) CheckWhen(grid,2500)}
        {"50 mil" SetValue(Grid,5000) CheckWhen(grid,5000)}
        {"100 mil" SetValue(Grid,10000) CheckWhen(grid,10000)}
        {"increment by 5 mil" SetValue(Grid,+500)}
        {"decrement by 5 mil" SetValue(Grid,-500)}
        {"increment by 1 mm" SetValue(Grid,+393.7007874)}
        {"decrement by 1 mm" SetValue(Grid,-393.7007874)}
        -
        {"Zoom Setting..." foreground=red sensitive=false vertSpace=60}
        {"4 : 1" GetXY(Select a point) SetValue(Zoom,=-4) CheckWhen(zoom,-4)}
        {"2 : 1" GetXY(Select a point) SetValue(Zoom,=-2) CheckWhen(zoom,-2)}
        {"1 : 1" GetXY(Select a point) SetValue(Zoom,0) CheckWhen(zoom,0)}
        {"1 : 2" GetXY(Select a point) SetValue(Zoom,2) CheckWhen(zoom,2)}
        {"1 : 4" GetXY(Select a point) SetValue(Zoom,4) CheckWhen(zoom,4)}
        {"1 : 8" GetXY(Select a point) SetValue(Zoom,6) CheckWhen(zoom,6)}
        {"1 : 16" GetXY(Select a point) SetValue(Zoom,8) CheckWhen(zoom,8)}
        -
        {"Displayed element-name..." foreground=red sensitive=false vertSpace=60}
        {"Description" Display(Description) CheckWhen(elementname,1)}
        {"Name on PCB" Display(NameOnPCB) CheckWhen(elementname,2)}
        {"Value" Display(Value) CheckWhen(elementname,3)}
        -
        {"Pinout shows number" CheckWhen(shownumber) Display(ToggleName)}
        {"Open pinout menu" GetXY(Select an element) Display(Pinout)}
        }
    
    {Sizes
        @sizes
        {"Adjust active sizes ..." AdjustStyle(0)}
        }
    
    {Settings
        {"Layer groups" foreground=red sensitive=false vertSpace=60}
        {"Edit layer groupings" EditLayerGroups()}
        -
        {"'All-direction' lines" CheckWhen(alldirection) Display(Toggle45Degree)}
        {"Auto swap line start angle" CheckWhen(swapstartdir) Display(ToggleStartDirection)}
        {"Orthogonal moves" CheckWhen(orthomove) Display(ToggleOrthoMove)}
        {"Crosshair snaps to pins and pads" CheckWhen(snappin) Display(ToggleSnapPin)}
        {"Crosshair shows DRC clearance" CheckWhen(showdrc) Display(ToggleShowDRC)}
        {"Auto enforce DRC clearance" CheckWhen(autodrc) Display(ToggleAutoDRC)}
        -
        {"Rubber band mode" CheckWhen(rubberband) Display(ToggleRubberBandMode)}
        {"Require unique element names" CheckWhen(uniquename) Display(ToggleUniqueNames)}
        {"Auto-zero delta measurements" CheckWhen(localref) Display(ToggleLocalRef)}
        {"New lines, arcs clear polygons" CheckWhen(clearnew) Display(ToggleClearLine)}
        {"Thin draw" CheckWhen(thindraw) Display(ToggleThindraw)}
        {"Check polygons" CheckWhen(checkplanes) Display(ToggleCheckPlanes)}
        }
    
    {Select
        {"Select all objects" Select(All)}
        {"Select all connected objects" Select(Connection)}
        -
        {"Unselect all objects" Unselect(All)}
        {"unselect all connected objects" Unselect(Connection)}
        -
        {"Select by name" foreground=red sensitive=false vertSpace=60}
        {"All objects" Select(ObjectByName) ActiveWhen(have_regex)}
        {"Elements" Select(ElementByName) ActiveWhen(have_regex)}
        {"Pads" Select(PadByName) ActiveWhen(have_regex)}
        {"Pins" Select(PinByName) ActiveWhen(have_regex)}
        {"Text Objects" Select(TextByName) ActiveWhen(have_regex)}
        {"Vias" Select(ViaByName) ActiveWhen(have_regex)}
        -
        {"Auto-place selected elements" AutoPlaceSelected()}
        {"Move selected elements to other side" Flip(SelectedElements)}
        {"Delete selected objects" RemoveSelected()}
        {"Convert selection to element" Select(Convert)}
        -
        {"Optimize selected rats" DeleteRats(SelectedRats) AddRats(SelectedRats)}
        {"Auto-route selected rats" AutoRoute(SelectedRats)}
        {"Rip-up selected auto-routed tracks" RipUp(Selected)}
        -
        {"Change size of selected objects" foreground=red sensitive=false vertSpace=60}
        {"Lines -10 mil" ChangeSize(SelectedLines,-10,mil)}
        {"Lines +10 mil" ChangeSize(SelectedLines,-10,mil)}
        {"Pads -10 mil" ChangeSize(SelectedPads,-10,mil)}
        {"Pads +10 mil" ChangeSize(SelectedPads,-10,mil)}
        {"Pins -10 mil" ChangeSize(SelectedPins,-10,mil)}
        {"Pins +10 mil" ChangeSize(SelectedPins,-10,mil)}
        {"Texts -10 mil" ChangeSize(SelectedTexts,-10,mil)}
        {"Texts +10 mil" ChangeSize(SelectedTexts,-10,mil)}
        {"Vias -10 mil" ChangeSize(SelectedVias,-10,mil)}
        {"Vias +10 mil" ChangeSize(SelectedVias,-10,mil)}
        -
        {"Change drilling hole of selected objects" foreground=red sensitive=false vertSpace=60}
        {"Vias -10 mil" ChangeDrillSize(SelectedVias,-10,mil)}
        {"Vias +10 mil" ChangeDrillSize(SelectedVias,+10,mil)}
        {"Pins -10 mil" ChangeDrillSize(SelectedPins,-10,mil)}
        {"Pins +10 mil" ChangeDrillSize(SelectedPins,+10,mil)}
        -
        {"Change square-flag of selected objects" foreground=red sensitive=false vertSpace=60}
        {"Elements" ChangeSquare(SelectedElements)}
        {"Pins" ChangeSquare(SelectedPins)}
        }
    
    {Buffer
        {"Copy selection to buffer" GetXY(Press a button at the element's location)
            PasteBuffer(Clear) PasteBuffer(AddSelected) Mode(PasteBuffer)}
        {"Cut selection to buffer" GetXY(Press a button at the element's location)
            PasteBuffer(Clear) PasteBuffer(AddSelected) RemoveSelected() Mode(PasteBuffer)}
        {"Paste buffer to layout" Mode(PasteBuffer)}
        -
        {"Rotate buffer 90 deg CCW" Mode(PasteBuffer) PasteBuffer(Rotate,1)}
        {"Rotate buffer 90 deg CW" Mode(PasteBuffer) PasteBuffer(Rotate,3)}
        {"Mirror buffer (up/down)" Mode(PasteBuffer) PasteBuffer(Mirror)}
        {"Mirror buffer (left/right)" Mode(PasteBuffer) PasteBuffer(Rotate,1)
            PasteBuffer(Mirror) PasteBuffer(Rotate,3)}
        -
        {"Clear buffer" PasteBuffer(Clear)}
        {"Convert buffer to element" PasteBuffer(Convert)}
        {"Break buffer elements to pieces" PasteBuffer(Restore)}
        {"Save buffer elements to file" PasteBuffer(Save)}
        -
        {"Select current buffer" foreground=red sensitive=false vertSpace=60}
        {"#1" CheckWhen(buffer,1) PasteBuffer(1)}
        {"#2" CheckWhen(buffer,2) PasteBuffer(2)}
        {"#3" CheckWhen(buffer,3) PasteBuffer(3)}
        {"#4" CheckWhen(buffer,4) PasteBuffer(4)}
        {"#5" CheckWhen(buffer,5) PasteBuffer(5)}
        }
    
    {Connects
        {"Lookup connection to object" GetXY(Select the object) Connection(Find)}
        {"Reset scanned pads/pins/vias" Connection(ResetPinsViasAndPads) Display(Redraw)}
        {"Reset scanned lines/polygons" Connection(ResetLinesAndPolygons) Display(Redraw)}
        {"Reset all connections" Connection(Reset) Display(Redraw)}
        -
        {"Optimize rats-nest" DeleteRats(AllRats) AddRats(AllRats)}
        {"Erase rats-nest" DeleteRats(AllRats)}
        -
        {"Auto-route selected rats" AutoRoute(Selected)}
        {"Auto-route all rats" AutoRoute(AllRats)}
        {"Rip up all auto-routed tracks" RipUp(All)}
        -
        {"Auto-Optimize" djopt(auto) Display(ClearAndRedraw)}
        {"Debumpify" djopt(debumpify) Display(ClearAndRedraw)}
        {"Unjaggy" djopt(unjaggy) Display(ClearAndRedraw)}
        {"Vianudge" djopt(vianudge) Display(ClearAndRedraw)}
        {"Viatrim" djopt(viatrim) Display(ClearAndRedraw)}
        {"Orthopull" djopt(orthopull) Display(ClearAndRedraw)}
        {"SimpleOpts" djopt(simple) Display(ClearAndRedraw)}
        {"Miter" djopt(miter) Display(ClearAndRedraw)}
        {"Only autorouted nets" OptAutoOnly() CheckWhen(OptAutoOnly)}
        -
        {"Design Rule Checker" SetValue(Zoom,=-2) DRC()}
        }
    
    {Info
        {"Generate object report" GetXY(Select the object) Report(Object)}
        {"Generate drill summary" Report(DrillReport)}
        {"Report found pins/pads" Report(FoundPins)}
        }
    
    {Window
        {"Board Layout" DoWindows(1)}
        {"Library" DoWindows(2)}
        {"Message Log" DoWindows(3) ActiveWhen(uselogwindow)}
        {"Netlist" DoWindows(4) ActiveWhen(netlist)}
        }
    }
