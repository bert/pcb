# Note - pcb-menu.res is used to build pcb-menu.h

# This is the main menu bar
MainMenu = {
    {File
        {"About..." About()}
        {"Save layout" Save(Layout)}
        {"Save layout as..." Save(LayoutAs)}
        {"Load layout" Load(Layout)}
        {"Load element data to paste-buffer" PasteBuffer(Clear) Load(ElementTobuffer)}
        {"Load layout data to paste-buffer" PasteBuffer(Clear) Load(LayoutTobuffer)}
        {"Load netlist file" Load(Netlist)}
        {"Load vendor resource file" LoadVendor()}
        {"Print layout..." Print() ActiveWhen(DataNonEmpty)}
        -
        {"Save connection data of..." foreground=red sensitive=false}
        {" a single element" GetXY(press a button at the element's location) Save(ElementConnections)}
        {" all elements" Save(AllConnections)}
        {" unused pins" Save(AllUnusedPins)}
        -
        {"Start new layout" New()}
        -
        {"Quit Program" Quit() a={"Alt-Q" "Mod1<Key>q"}}
        }
    
    {Edit
        {"Undo last operation" Undo() a={"U" "<Key>u"}}
        {"Redo last undone operation" Redo() a={"Shift-R" "Shift<Key>r"}}
        {"Clear undo-buffer" Undo(ClearList) a={"Shift-Ctrl-U" "Shift Ctrl<Key>u"}}
        -
        {"Cut selection to buffer" GetXY(Press a button at the element's location)
            PasteBuffer(Clear) PasteBuffer(AddSelected) RemoveSelected() Mode(PasteBuffer)
	    a={"Shift-Ctrl-X" "Shift Ctrl<Key>x"}}
        {"Copy selection to buffer" GetXY(Press a button at the element's location)
            PasteBuffer(Clear) PasteBuffer(AddSelected) Mode(PasteBuffer)
	    a={"Ctrl-X" "Ctrl<Key>x"}}
        {"Paste buffer to layout" Mode(PasteBuffer) a={"F7" "<Key>F7"}}
        -
        {"Unselect all" Unselect(All) a={"Shift-Alt-A" "Shift Mod1<Key>a"}}
        {"Select all" Select(All) a={"Alt-A" "Alt<Key>a"}}
        -
        {"Edit Names..." foreground=red sensitive=false}
        {" Change text on layout" ChangeName(Object) a={"N" "<Key>n"}}
        {" Edit name of layout" ChangeName(Layout)}
        {" Edit name of active layer" ChangeName(Layer)}
        -
        {"Change board size..." AdjustStyle(0)}
        }
    
    {Screen
        {"Redraw layout" Display(ClearAndRedraw) a={"R" "<Key>r"}}
        {"Center layout" Display(Center) a={"C" "<Key>c"}}
        -
        {"Display grid" CheckWhen(drawgrid) ActiveWhen(gridfactor) Display(Grid)}
        {"Realign grid" GetXY(Press a button at a grid point) Display(ToggleGrid)}
        {"View solder side" CheckWhen(showsolderside) GetXY(Press a button at the desired point) SwapSides()
		a={"Tab" "<Key>Tab"}}
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
        {"increment by 5 mil" SetValue(Grid,+500) a={"G" "<Key>g"}}
        {"decrement by 5 mil" SetValue(Grid,-500) a={"Shift-G" "Shift<Key>g"}}
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
        {"Reference Designator" Display(NameOnPCB) CheckWhen(elementname,2)}
        {"Value" Display(Value) CheckWhen(elementname,3)}
        -
        {"Pinout shows number" CheckWhen(shownumber) Display(ToggleName)}
        {"Open pinout menu" GetXY(Select an element) Display(Pinout) a={"Shift-D" "Shift<Key>d"}}
        }
    
    {Sizes
        @sizes
        {"Adjust active sizes ..." AdjustStyle(0)}
        }
    
    {Settings
        {"Layer groups" foreground=red sensitive=false vertSpace=60}
        {"Edit layer groupings" EditLayerGroups()}
        -
        {"'All-direction' lines" CheckWhen(alldirection) Display(Toggle45Degree) a={"." "<Key>."}}
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
	{"Show autorouter trials" CheckWhen(liveroute) Display(ToggleLiveRoute)}
        {"Thin draw" CheckWhen(thindraw) Display(ToggleThindraw) a={"|" "<Key>|"}}
        {"Check polygons" CheckWhen(checkplanes) Display(ToggleCheckPlanes)}
        -
	{"Enable vendor drill mapping" ToggleVendor() CheckWhen(VendorMapOn)}
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
        {"Auto-place selected elements" AutoPlaceSelected() a={"Ctrl-P" "Ctrl<Key>p"}}
        {"Disperse all elements" DisperseElements(All)}
        {"Disperse selected elements" DisperseElements(Selected)}
        {"Move selected elements to other side" Flip(SelectedElements) a={"Shift-B" "Shift<Key>b"}}
        {"Delete selected objects" RemoveSelected()}
        {"Convert selection to element" Select(Convert)}
        -
        {"Optimize selected rats" DeleteRats(SelectedRats) AddRats(SelectedRats)}
        {"Auto-route selected rats" AutoRoute(SelectedRats) a={"Alt-R" "Mod1<Key>r"}}
        {"Rip-up selected auto-routed tracks" RipUp(Selected)}
        -
        {"Change size of selected objects" foreground=red sensitive=false vertSpace=60}
        {"Lines -10 mil" ChangeSize(SelectedLines,-10,mil)}
        {"Lines +10 mil" ChangeSize(SelectedLines,+10,mil)}
        {"Pads -10 mil" ChangeSize(SelectedPads,-10,mil)}
        {"Pads +10 mil" ChangeSize(SelectedPads,+10,mil)}
        {"Pins -10 mil" ChangeSize(SelectedPins,-10,mil)}
        {"Pins +10 mil" ChangeSize(SelectedPins,+10,mil)}
        {"Texts -10 mil" ChangeSize(SelectedTexts,-10,mil)}
        {"Texts +10 mil" ChangeSize(SelectedTexts,+10,mil)}
        {"Vias -10 mil" ChangeSize(SelectedVias,-10,mil)}
        {"Vias +10 mil" ChangeSize(SelectedVias,+10,mil)}
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
        {"#1" CheckWhen(buffer,1) PasteBuffer(1) a={"Shift-1" "Shift<Key>1"}}
        {"#2" CheckWhen(buffer,2) PasteBuffer(2) a={"Shift-2" "Shift<Key>2"}}
        {"#3" CheckWhen(buffer,3) PasteBuffer(3) a={"Shift-3" "Shift<Key>3"}}
        {"#4" CheckWhen(buffer,4) PasteBuffer(4) a={"Shift-4" "Shift<Key>4"}}
        {"#5" CheckWhen(buffer,5) PasteBuffer(5) a={"Shift-5" "Shift<Key>5"}}
        }
    
    {Connects
        {"Lookup connection to object" GetXY(Select the object) Connection(Find) a={"Ctrl-F" "Ctrl<Key>f"}}
        {"Reset scanned pads/pins/vias" Connection(ResetPinsViasAndPads) Display(Redraw)}
        {"Reset scanned lines/polygons" Connection(ResetLinesAndPolygons) Display(Redraw)}
        {"Reset all connections" Connection(Reset) Display(Redraw) a={"Shift-F" "Shift<Key>f"}}
        -
        {"Optimize rats-nest" Atomic(Save) DeleteRats(AllRats)
		Atomic(Restore) AddRats(AllRats) Atomic(Block) a={"O" "<Key>o"}}
        {"Erase rats-nest" DeleteRats(AllRats) a={"E" "<Key>e"}}
        -
        {"Auto-route selected rats" AutoRoute(Selected)}
        {"Auto-route all rats" AutoRoute(AllRats)}
        {"Rip up all auto-routed tracks" RipUp(All)}
        -
        {"Auto-Optimize" djopt(auto) Display(ClearAndRedraw) a={"Shift-=" "Shift<Key>="}}
        {"Debumpify" djopt(debumpify) Display(ClearAndRedraw)}
        {"Unjaggy" djopt(unjaggy) Display(ClearAndRedraw)}
        {"Vianudge" djopt(vianudge) Display(ClearAndRedraw)}
        {"Viatrim" djopt(viatrim) Display(ClearAndRedraw)}
        {"Orthopull" djopt(orthopull) Display(ClearAndRedraw)}
        {"SimpleOpts" djopt(simple) Display(ClearAndRedraw) a={"=" "<Key>="}}
        {"Miter" djopt(miter) Display(ClearAndRedraw)}
        {"Only autorouted nets" OptAutoOnly() CheckWhen(OptAutoOnly)}
        -
        {"Design Rule Checker" DRC()}
        -
	{"Apply vendor drill mapping" ApplyVendor()}
        }
    
    {Info
        {"Generate object report" GetXY(Select the object) Report(Object) a={"Ctrl-R" "Ctrl<Key>r"}}
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
