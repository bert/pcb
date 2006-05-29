# -*- c -*-
# Note - pcb-menu.res is used to build pcb-menu.h
# Note - parameters are sensitive to extra spaces around the commas

MainMenu =
{
  {File
   {"About..." About()}
   {"Save layout" Save(Layout)}
   {"Save layout as..." Save(LayoutAs)}
	{"Revert" Load(Revert,none)}
   {"Load layout" Load(Layout)}
   {"Load element data to paste-buffer" PasteBuffer(Clear) Load(ElementTobuffer)}
   {"Load layout data to paste-buffer" PasteBuffer(Clear) Load(LayoutTobuffer)}
   {"Load netlist file" Load(Netlist)}
   {"Load vendor resource file" LoadVendor()}
   {"Print layout..." Print()}
   {"Export layout..." Export()}
   -
   {"Save connection data of..." foreground=grey50 sensitive=false}
   {" a single element" GetXY(press a button at the element location) Save(ElementConnections)}
   {" all elements" Save(AllConnections)}
   {" unused pins" Save(AllUnusedPins)}
   -
   {"Start new layout" New()}
   -
   {"Quit Program" Quit() m=Q a={"Ctrl-Q" "Ctrl<Key>q"}}
  }
  {View
   {"Flip up/down" checked=flip_y Flip(V) a={"Tab" "<Key>Tab"}}
   {"Flip left/right" checked=flip_x Flip(H) a={"Shift-Tab" "Shift<Key>Tab"}}
   {"Spin 180°" Flip(H) Flip(V) a={"Ctrl-Tab" "Ctrl<Key>Tab"}}
   {"Show soldermask" checked=showmask Display(ToggleMask)}
   -
   {"Displayed element-name..." foreground=grey50 sensitive=false}
   {"Description" Display(Description) checked=elementname,1}
   {"Reference Designator" Display(NameOnPCB) checked=elementname,2}
   {"Value" Display(Value) checked=elementname,3}
   -
   {"Pinout shows number" checked=shownumber Display(ToggleName)}
   {"Open pinout menu" Display(Pinout) a={"Shift-D" "Shift<Key>d"}}
   -
   {Zoom
    {"Zoom In 2X" Zoom(-2)}
    {"Zoom In 20%" Zoom(-1.2) m=Z a={"Z" "<Key>z"}}
    {"Zoom Out 20%" Zoom(+1.2) m=O a={"Shift-Z" "Shift<Key>z"}}
    {"Zoom Out 2X" Zoom(+2)}
    {"Zoom Max" Zoom() m=M a={"V" "<Key>v"}}
    -
    {"Zoom to 0.1mil/px" Zoom(=10)}
    {"Zoom to 0.01mm/px" Zoom(=39.37)}
    {"Zoom to 1mil/px" Zoom(=100)}
    {"Zoom to 0.05mm/px" Zoom(=196.8504)}
    {"Zoom to 2.5mil/px" Zoom(=250)}
    {"Zoom to 0.1mm/px" Zoom(=393.7)}
    {"Zoom to 10mil/px" Zoom(=1000)}
   }
   {Grid
    {"mils" checked=grid_units_mm,0 SetUnits(mil)}
    {"mms"  checked=grid_units_mm,1 SetUnits(mm)}
    {"Display grid" checked=drawgrid Display(Grid)}
    {"Realign grid" GetXY(Press a button at a grid point) Display(ToggleGrid)}
    {"No Grid" checked=gridsize,1 SetValue(Grid,1)}
    -
    {  "0.1 mil" checked=gridsize,10 SetUnits(mil) SetValue(Grid,10)}
    {  "1 mil"   checked=gridsize,100 SetUnits(mil) SetValue(Grid,100)}
    {  "5 mil"   checked=gridsize,500 SetUnits(mil) SetValue(Grid,500)}
    { "10 mil"   checked=gridsize,1000 SetUnits(mil) SetValue(Grid,1000)}
    { "25 mil"   checked=gridsize,2500 SetUnits(mil) SetValue(Grid,2500)}
    {"100 mil"   checked=gridsize,10000 SetUnits(mil) SetValue(Grid,10000)}
    -
    {"0.01 mm" checked=gridsize,39 SetUnits(mm) SetValue(Grid,39.37007874)}
    {"0.05 mm" checked=gridsize,197 SetUnits(mm) SetValue(Grid,196.85039370)}
    {"0.1 mm"  checked=gridsize,394 SetUnits(mm) SetValue(Grid,393.70078740)}
    {"0.25 mm" checked=gridsize,984 SetUnits(mm) SetValue(Grid,984.25197)}
    {"0.5 mm"  checked=gridsize,1969 SetUnits(mm) SetValue(Grid,1968.503937)}
    {"1 mm"    checked=gridsize,3937 SetUnits(mm) SetValue(Grid,3937.00787400)}
    -
    {"Grid -5mil" SetValue(Grid,-5,mil) a={"Shift-G" "Shift<Key>g"}}
    {"Grid +5mil" SetValue(Grid,+5,mil) a={"G" "<Key>g"}}
    {"Grid -0.05mm" SetValue(Grid,-0.05,mm) a={"Shift-Ctrl-G" "Shift Ctrl<Key>g"}}
    {"Grid +0.05mm" SetValue(Grid,+0.05,mm) a={"Ctrl-G" "Ctrl<Key>g"}}
   }
   -
   {"Shown Layers"
    @layerview
    -
    {"Edit Layer Groups" EditLayerGroups()}
   }
   {"Current Layer"
    @layerpick
   }
  }
  {Edit
   {"Undo last operation" Undo() a={"U" "<Key>u"}}
   {"Redo last undone operation" Redo() a={"Shift-R" "Shift<Key>r"}}
   {"Clear undo-buffer" Undo(ClearList) a={"Shift-Ctrl-U" "Shift Ctrl<Key>u"}}
   -
   {"Cut selection to buffer" GetXY(Press a button at the reference location)
    PasteBuffer(Clear) PasteBuffer(AddSelected) RemoveSelected() Mode(PasteBuffer)
    a={"Ctrl-C" "Ctrl<Key>c"}}
   {"Copy selection to buffer" GetXY(Press a button at the reference location)
    PasteBuffer(Clear) PasteBuffer(AddSelected) Mode(PasteBuffer)
    a={"Ctrl-X" "Ctrl<Key>x"}}
   {"Paste buffer to layout" Mode(PasteBuffer) a={"F7" "<Key>F7"}}
   -
   {"Unselect all" Unselect(All) a={"Shift-Alt-A" "Shift Alt<Key>a"}}
   {"Select all" Select(All) a={"Alt-A" "Alt<Key>a"}}
   -
   {"Move to current layer" MoveToCurrentLayer(Object) a={"M" "<Key>m"}}
   {"Move selected to current layer" MoveToCurrentLayer(Selected) a={"Shift-M" "Shift<Key>m"}}
   -
   {"Edit Names..." foreground=grey50 sensitive=false}
   {" Change text on layout" ChangeName(Object) a={"N" "<Key>n"}}
   {" Edit name of layout" ChangeName(Layout)}
   {" Edit name of active layer" ChangeName(Layer)}
   -
   {"Board Sizes" AdjustSizes()}
   {"Route Styles"
    @routestyles
    -
    {"Edit..." AdjustStyle(0)}
   }
  }
  {Tools
   {"None" checked=nomode,1 Mode(None)}
   {"Via" checked=viamode,1 Mode(Via) a={"F1" "<Key>F1"}}
   {"Line" checked=linemode,1 Mode(Line) a={"F2" "<Key>F2"}}
   {"Arc" checked=arcmode,1 Mode(Arc) a={"F3" "<Key>F3"}}
   {"Text" checked=textmode,1 Mode(Text) a={"F4" "<Key>F4"}}
   {"Rectangle" checked=rectanglemode,1 Mode(Rectangle) a={"F5" "<Key>F5"}}
   {"Polygon" checked=polygonmode,1 Mode(Polygon) a={"F6" "<Key>F6"}}
   {"Buffer" checked=pastebuffermode,1 Mode(PasteBuffer) a={"F7" "<Key>F7"}}
   {"Remove" checked=removemode,1 Mode(Remove) a={"F8" "<Key>F8"}}
   {"Rotate" checked=rotatemode,1 Mode(Rotate) a={"F9" "<Key>F9"}}
   {"Thermal" checked=thermalmode,1 Mode(Thermal) a={"F10" "<Key>F10"}}
   {"Arrow" checked=arrowmode,1 Mode(Arrow)  a={"F11" "<Key>F11"}}
   {"Insert Point" checked=insertpointmode,1 Mode(InsertPoint) a={"Insert" "<Key>Insert"}}
   {"Move" checked=movemode,1 Mode(Move)}
   {"Copy" checked=copymode,1 Mode(Copy)}
   {"Lock" checked=lockmode,1 Mode(Lock)}
   {"Cancel" Mode(Cancel) a={"Esc" "<Key>Escape"}}
   -
   {"Command" Command() a={":" "<Key>:"}}
  }
  {Settings
   {"Layer groups" foreground=grey50 sensitive=false}
   {"Edit layer groupings" EditLayerGroups()}
   -
   {"'All-direction' lines" checked=alldirection Display(Toggle45Degree) a={"." "<Key>."}}
   {"Auto swap line start angle" checked=swapstartdir Display(ToggleStartDirection)}
   {"Orthogonal moves" checked=orthomove Display(ToggleOrthoMove)}
   {"Crosshair snaps to pins and pads" checked=snappin Display(ToggleSnapPin)}
   {"Crosshair shows DRC clearance" checked=showdrc Display(ToggleShowDRC)}
   {"Auto enforce DRC clearance" checked=autodrc Display(ToggleAutoDRC)}
   -
   {"Rubber band mode" checked=rubberband Display(ToggleRubberBandMode)}
   {"Require unique element names" checked=uniquename Display(ToggleUniqueNames)}
   {"Auto-zero delta measurements" checked=localref Display(ToggleLocalRef)}
   {"New lines, arcs clear polygons" checked=clearnew Display(ToggleClearLine)}
   {"Show autorouter trials" checked=liveroute Display(ToggleLiveRoute)}
   {"Thin draw" checked=thindraw Thindraw() a={"|" "<Key>|"}}
   {"Thin draw poly" checked=thindrawpoly ThindrawPoly() a={"Ctrl-Shift-P" "Ctrl Shift<Key>p"}}
   {"Check polygons" checked=checkplanes Display(ToggleCheckPlanes)}
   -
   {"Pinout shows number" checked=shownumber Display(ToggleName)}
   {"Pins/Via show Name/Number" Display(PinOrPadName) a={"D" "<Key>d"}}
   {"Enable vendor drill mapping" ToggleVendor() checked=VendorMapOn}
  }
    
  {Select
   {"Select all objects" Select(All)}
   {"Select all connected objects" Select(Connection)}
   -
   {"Unselect all objects" Unselect(All)}
   {"unselect all connected objects" Unselect(Connection)}
   -
   {"Select by name" foreground=grey50 sensitive=false}
   {"All objects" Select(ObjectByName) ActiveWhen(have_regex)}
   {"Elements" Select(ElementByName) ActiveWhen(have_regex)}
   {"Pads" Select(PadByName) ActiveWhen(have_regex)}
   {"Pins" Select(PinByName) ActiveWhen(have_regex)}
   {"Text Objects" Select(TextByName) ActiveWhen(have_regex)}
   {"Vias" Select(ViaByName) ActiveWhen(have_regex)}
   -
   {"Auto-place selected elements" AutoPlaceSelected() a={"Ctrl-P" "Ctrl<Key>p"}}
   {"Disperse all elements" DisperseElements()}
   {"Move selected elements to other side" Flip(SelectedElements) a={"Shift-B" "Shift<Key>b"}}
   {"Delete selected objects" RemoveSelected()}
   {"Convert selection to element" Select(Convert)}
   -
   {"Optimize selected rats" DeleteRats(SelectedRats) AddRats(SelectedRats)}
   {"Auto-route selected rats" AutoRoute(SelectedRats) a={"Alt-R" "Alt<Key>r"}}
   {"Rip-up selected auto-routed tracks" RipUp(Selected)}
   -
   {"Change size of selected objects" foreground=grey50 sensitive=false}
   {"Lines -10 mil" ChangeSize(SelectedLines,-10,mil) ChangeSize(SelectedArcs,-10,mil)}
   {"Lines +10 mil" ChangeSize(SelectedLines,+10,mil) ChangeSize(SelectedArcs,+10,mil)}
   {"Pads -10 mil" ChangeSize(SelectedPads,-10,mil)}
   {"Pads +10 mil" ChangeSize(SelectedPads,+10,mil)}
   {"Pins -10 mil" ChangeSize(SelectedPins,-10,mil)}
   {"Pins +10 mil" ChangeSize(SelectedPins,+10,mil)}
   {"Texts -10 mil" ChangeSize(SelectedTexts,-10,mil)}
   {"Texts +10 mil" ChangeSize(SelectedTexts,+10,mil)}
   {"Vias -10 mil" ChangeSize(SelectedVias,-10,mil)}
   {"Vias +10 mil" ChangeSize(SelectedVias,+10,mil)}
   -
   {"Change drilling hole of selected objects" foreground=grey50 sensitive=false}
   {"Vias -10 mil" ChangeDrillSize(SelectedVias,-10,mil)}
   {"Vias +10 mil" ChangeDrillSize(SelectedVias,+10,mil)}
   {"Pins -10 mil" ChangeDrillSize(SelectedPins,-10,mil)}
   {"Pins +10 mil" ChangeDrillSize(SelectedPins,+10,mil)}
   -
   {"Change square-flag of selected objects" foreground=grey50 sensitive=false}
   {"Elements" ChangeSquare(SelectedElements)}
   {"Pins" ChangeSquare(SelectedPins)}
  }
    
  {Buffer
   {"Copy selection to buffer" GetXY(Press a button at the element location)
    PasteBuffer(Clear) PasteBuffer(AddSelected) Mode(PasteBuffer)}
   {"Cut selection to buffer" GetXY(Press a button at the element location)
    PasteBuffer(Clear) PasteBuffer(AddSelected) RemoveSelected() Mode(PasteBuffer)}
   {"Paste buffer to layout" Mode(PasteBuffer)}
   -
   {"Rotate buffer 90 deg CCW" Mode(PasteBuffer) PasteBuffer(Rotate,1)
    a={"Shift-F7" "Shift<Key>F7"}}
   {"Rotate buffer 90 deg CW" Mode(PasteBuffer) PasteBuffer(Rotate,3)}
   {"Mirror buffer (up/down)" Mode(PasteBuffer) PasteBuffer(Mirror)}
   {"Mirror buffer (left/right)" Mode(PasteBuffer) PasteBuffer(Rotate,1)
    PasteBuffer(Mirror) PasteBuffer(Rotate,3)}
   -
   {"Clear buffer" PasteBuffer(Clear)}
   {"Convert buffer to element" PasteBuffer(Convert)}
   {"Break buffer elements to pieces" PasteBuffer(Restore)}
   {"Save buffer elements to file" Save(PasteBuffer)}
   -
   {"Select current buffer" foreground=grey50 sensitive=false}
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
   {"Erase selected rats" DeleteRats(SelectedRats) a={"Shift-E" "Shift<Key>e"}}
   -
   {"Auto-route selected rats" AutoRoute(Selected)}
   {"Auto-route all rats" AutoRoute(AllRats)}
   {"Rip up all auto-routed tracks" RipUp(All)}
   -
   {"Auto-Optimize" djopt(auto)  a={"Shift-=" "Shift<Key>="}}
   {"Debumpify" djopt(debumpify) }
   {"Unjaggy" djopt(unjaggy) }
   {"Vianudge" djopt(vianudge) }
   {"Viatrim" djopt(viatrim) }
   {"Orthopull" djopt(orthopull) }
   {"SimpleOpts" djopt(simple)  a={"=" "<Key>="}}
   {"Miter" djopt(miter) }
   {"Puller" a={"Y" "<Key>y"} Puller() }
   {"Only autorouted nets" OptAutoOnly() checked=optautoonly}
   -
   {"Design Rule Checker" DRC()}
   -
   {"Apply vendor drill mapping" ApplyVendor()}
  }
    
  {Info
   {"Generate object report" ReportObject() a={"Ctrl-R" "Ctrl<Key>r"}}
   {"Generate drill summary" Report(DrillReport)}
   {"Report found pins/pads" Report(FoundPins)}
   {"Key Bindings"
    {"Remove" a={"Backspace" "<Key>BackSpace"}
     Mode(Save)
     Mode(Remove)
     Mode(Notify)
     Mode(Restore)
    }
    {"Remove" a={"Delete" "<Key>Delete"}
     Mode(Save)
     Mode(Remove)
     Mode(Notify)
     Mode(Restore)
    }
    {"Remove Connected" a={"Shift-Backspace" "Shift<Key>BackSpace"}
     Atomic(Save)
     Connection(Reset)
     Atomic(Restore)
     Unselect(All)
     Atomic(Restore)
     Connection(Find)
     Atomic(Restore)
     Select(Connection)
     Atomic(Restore)
     RemoveSelected()
     Atomic(Restore)
     Connection(Reset)
     Atomic(Restore)
     Unselect(All)
     Atomic(Block)
    }
    {"Remove Connected" a={"Shift-Delete" "Shift<Key>Delete"}
     Atomic(Save)
     Connection(Reset)
     Atomic(Restore)
     Unselect(All)
     Atomic(Restore)
     Connection(Find)
     Atomic(Restore)
     Select(Connection)
     Atomic(Restore)
     RemoveSelected()
     Atomic(Restore)
     Connection(Reset)
     Atomic(Restore)
     Unselect(All)
     Atomic(Block)
    }
    {"Set Same" a={"A" "<Key>a"} SetSame()}
    {"Flip Object" a={"B" "<Key>b"} Flip(Object)}
    {"Find Connections" a={"F" "<Key>f"} Connection(Reset) Connection(Find)}
    {"ToggleHideName Object" a={"H" "<Key>h"} ToggleHideName(Object)}
    {"ToggleHideName SelectedElement" a={"Shift-H" "Shift<Key>h"} ToggleHideName(SelectedElements)}
    {"ChangeHole Object" a={"Ctrl-H" "Ctrl<Key>h"} ChangeHole(Object)}
    {"ChangeJoin Object" a={"J" "<Key>j"} ChangeJoin(Object)}
    {"ChangeJoin SelectedObject" a={"Shift-J" "Shift<Key>j"} ChangeJoin(SelectedObjects)}
    {"Clear Object +2 mil" a={"K" "<Key>k"} ChangeClearSize(Object,+2,mil)}
    {"Clear Object -2 mil" a={"Shift-K" "Shift<Key>k"} ChangeClearSize(Object,-2,mil)}
    {"Clear Selected +2 mil" a={"Ctrl-K" "Ctrl<Key>k"} ChangeClearSize(SelectedObjects,+2,mil)}
    {"Clear Selected -2 mil" a={"Shift-Ctrl-K" "Shift Ctrl<Key>k"} ChangeClearSize(SelectedObjects,-2,mil)}
    {"Linesize +5 mil" a={"L" "<Key>l"} SetValue(LineSize,+5,mil)}
    {"Linesize -5 mil" a={"Shift-L" "Shift<Key>l"} SetValue(LineSize,-5,mil)}
    {"MarkCrosshair" a={"Ctrl-M" "Ctrl<Key>m"} MarkCrosshair()}
    {"Select shortest rat" a={"Shift-N" "Shift<Key>n"} AddRats(Close)}
    {"AddRats to selected pins" a={"Shift-O" "Shift<Key>o"}
     Atomic(Save)
     DeleteRats(AllRats)
     Atomic(Restore)
     AddRats(SelectedRats)
     Atomic(Block) }
    {"ChangeOctagon Object" a={"Ctrl-O" "Ctrl<Key>o"} ChangeOctagon(Object)}
    {"Polygon PreviousPoint" a={"P" "<Key>p"} Polygon(PreviousPoint)}
    {"Polygon Close" a={"Shift-P" "Shift<Key>p"} Polygon(Close)}
    {"ChangeSquare Object" a={"Q" "<Key>q"} ChangeSquare(Object)}
    {"ChangeSize +5 mil" a={"S" "<Key>s"} ChangeSize(Object,+5,mil)}
    {"ChangeSize -5 mil" a={"Shift-S" "Shift<Key>s"} ChangeSize(Object,-5,mil)}
    {"ChangeDrill +5 mil" a={"Alt-S" "Alt<Key>s"} ChangeDrillSize(Object,+5,mil)}
    {"ChangeDrill -5 mil" a={"Alt-Shift-S" "Alt Shift<Key>s"} ChangeDrillSize(Object,-5,mil)}
    {"TextScale +10 mil" a={"T" "<Key>t"} SetValue(TextScale,+10,mil)}
    {"TextScale -10 mil" a={"Shift-T" "Shift<Key>t"} SetValue(TextScale,-10,mil)}
    {"ViaSize +5 mil" a={"Ctrl-V" "Ctrl<Key>v"} SetValue(ViaSize,+5,mil)}
    {"ViaSize -5 mil" a={"Shift-Ctrl-V" "Shift Ctrl<Key>v"} SetValue(ViaSize,-5,mil)}
    {"ViaDrill +5 mil" a={"Alt-V" "Alt<Key>v"} SetValue(ViaDrillingHole,+5,mil)}
    {"ViaDrill -5 mil" a={"Alt-Shift-V" "Alt Shift<Key>v"} SetValue(ViaDrillingHole,-5,mil)}
    {"AddRats Selected" a={"Shift-W" "Shift<Key>w"} AddRats(SelectedRats)}
    {"Add All Rats" a={"W" "<Key>w"} AddRats(AllRats)}
    {"Undo" a={"Alt-Z" "Alt<Key>z"} Undo()}
    {"Cycle Clip" a={"/" "<Key>/"} Display(CycleClip)}
    {"Arrow" a={"Space" "<Key>space"} Mode(Arrow) checked=arrowmode,1}
    {"Temp Arrow ON" a={"[" "<Key>["} Mode(Save) Mode(Arrow) Mode(Notify)}
    {"Temp Arrow OFF" a={"]" "<Key>]"} Mode(Release) Mode(Restore)}
   }
  }
  {Window
   {"Board Layout" DoWindows(Layout)}
   {"Library" DoWindows(Library)}
   {"Message Log" DoWindows(Log)}
   {"Netlist" DoWindows(Netlist)}
   {"Pinout" Display(Pinout) a={"Shift-D" "Shift<Key>d"}}
  }
}
