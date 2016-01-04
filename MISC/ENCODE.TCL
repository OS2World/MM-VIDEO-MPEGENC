#!encode_sh -f

proc nothing {} {}

proc tail {aList} \
{
    set length [llength $aList]
    set index [expr $length-1]
    set entry [lindex $aList $index]

    return $entry
}


proc GetPSearch {} \
{
    global psearchVar

    if { $psearchVar == "pExhaustButton" } then \
	{ SetPSearch EXHAUSTIVE } \
    else \
    {
	if { $psearchVar == "pSubSampleButton" } then \
	     { SetPSearch SUBSAMPLE } \
	else \
	     { SetPSearch LOGARITHMIC }
    }
}


proc GetBSearch {} \
{
    global bsearchVar

    if { $bsearchVar == "bExhaustButton" } then \
	{ SetBSearch EXHAUSTIVE } \
    else \
    {
	if { $bsearchVar == "bSimpleButton" } then \
	     { SetBSearch SIMPLE } \
	else \
	     { SetBSearch CROSS2 }
    }
}


	###########
	# Q-SCALE #
	###########

proc GetQScale {} \
{
    set valueI [.qScaleI get]
    set valueP [.qScaleP get]
    set valueB [.qScaleB get]

    SetQScale $valueI $valueP $valueB
}


proc QScale {} \
{
    scale .qScaleI -orient horizontal -label "I Q-scale" -from 1 -to 31
    place .qScaleI -x 10 -y 640 -width 140

    scale .qScaleP -orient horizontal -label "P Q-scale" -from 1 -to 31
    place .qScaleP -x 180 -y 640 -width 140

    scale .qScaleB -orient horizontal -label "B Q-scale" -from 1 -to 31
    place .qScaleB -x 350 -y 640 -width 140

    .qScaleI set 8
    .qScaleP set 16
    .qScaleB set 25
}

	########################
	# MOTION VECTOR SEARCH #
	########################


proc MotionVectorSearch {} \
{
    global errorVar
    global pixelVar
    global psearchVar
    global bsearchVar
    global format

    radiobutton .buttonFull -text "Full Pixel" -variable pixelVar
    radiobutton .buttonHalf -text "Half Pixel" -variable pixelVar

    place .buttonFull -x 90 -y 470
    place .buttonHalf -x 90 -y 495

    .buttonFull select

    scale .searchScale -orient horizontal -label "Search Range" -from 2 -to 32
    place .searchScale -x 255 -y 470 -width 130
    .searchScale set 8

    label .psearchLabel -text "P-frame Search:"
    place .psearchLabel -x 10 -y 535

    radiobutton .pExhaustButton -text "EXHAUSTIVE" -variable psearchVar
    radiobutton .pSubSampleButton -text "SUBSAMPLE" -variable psearchVar
    radiobutton .pLogarithmicButton -text "LOGARITHMIC" -variable psearchVar

    place .pExhaustButton -x 10 -y 560
    place .pSubSampleButton -x 10 -y 585
    place .pLogarithmicButton -x 10 -y 610

    .pExhaustButton select

    label .bsearchLabel -text "B-frame Search:"
    place .bsearchLabel -x 255 -y 535

    radiobutton .bSimpleButton -text "SIMPLE" -variable bsearchVar
    radiobutton .bCrossButton -text "CROSS2" -variable bsearchVar
    radiobutton .bExhaustButton -text "EXHAUSTIVE" -variable bsearchVar

    place .bSimpleButton -x 255 -y 560
    place .bCrossButton -x 255 -y 585
    place .bExhaustButton -x 255 -y 610

    .bSimpleButton select
}


proc GetSearchRange {} \
{
    set value [.searchScale get]

    SetSearchRange $value
}

proc GetPixelType {} \
{
    global pixelVar

    if { $pixelVar == "buttonFull" } then \
	{ SetPixelSearch FULL } \
    else \
        { SetPixelSearch HALF }
}


	###########
	# PATTERN #
	###########

proc GetPattern {} \
{
    set patChar1 [tail [.pattern1 configure -text]]
    set patChar2 [tail [.pattern2 configure -text]]
    set patChar3 [tail [.pattern3 configure -text]]
    set patChar4 [tail [.pattern4 configure -text]]
    set patChar5 [tail [.pattern5 configure -text]]
    set patChar6 [tail [.pattern6 configure -text]]

    set patString $patChar1$patChar2$patChar3$patChar4$patChar5$patChar6

    SetPatternString $patString
}

proc ChangePattern {patternButton} \
{
    set temp [tail [$patternButton configure -text]]
    case $temp in I {set newTemp P} P {set newTemp B} B {set newTemp I}
    $patternButton configure -text $newTemp
}

proc PatternInterface {} \
{
    label .patternLabel -text "Pattern"

# no command for first -- must be an I-frame!!!
    button .pattern1 -text "I" -relief raised
    button .pattern2 -text "P" -relief raised -command { ChangePattern .pattern2 }
    button .pattern3 -text "P" -relief raised -command { ChangePattern .pattern3 }
    button .pattern4 -text "P" -relief raised -command { ChangePattern .pattern4 }
    button .pattern5 -text "P" -relief raised -command { ChangePattern .pattern5 }
    button .pattern6 -text "P" -relief raised -command { ChangePattern .pattern6 }

    place .patternLabel -x 10 -y 10
    place .pattern1 -x 110 -y 10
    place .pattern2 -x 130 -y 10
    place .pattern3 -x 150 -y 10
    place .pattern4 -x 170 -y 10
    place .pattern5 -x 190 -y 10
    place .pattern6 -x 210 -y 10
}


	#################
	# FILE BROWSING #
	#################

proc GetBrowseGlob {} \
{
    set temp [.fileBrowseEntry get]
    SetBrowseGlob $temp
}


proc ClearBrowseFile {} \
{
    .fileBrowseList delete 0 end
}


proc ShowCurrentDirectory {dirName} \
{
    set show "Directory:  $dirName"
    .pathLabel configure -text $show
}

proc BrowseDirectory {dirName} \
{
    if { [string last "/" $dirName] == -1 } then \
    {
	AddInputFile $dirName
    } else \
    {
	ChangeDirectory $dirName
	ClearBrowseFile
	GetBrowseGlob
	ListDirectory
    }
}

proc AddBrowseFile {fileName} \
{
    .fileBrowseList insert end $fileName
}


proc AddSelections {} \
{
    set selections [.fileBrowseList curselection]

    foreach index $selections {AddInputFile [.fileBrowseList get $index]}
}

proc AddAllSelections {} \
{
    set size [.fileBrowseList size]

    for {set index 0} {$index < $size} {set index [expr $index+1]} \
        {AddInputFile [.fileBrowseList get $index]}
}

proc FileBrowseInterface {} \
{
    label .pathLabel -text "."

    scrollbar .fileBrowseScroll -command ".fileBrowseList yview"
    listbox .fileBrowseList -yscroll ".fileBrowseScroll set"

    label .globLabel -text "Glob:"
    entry .fileBrowseEntry -width 150

    place .pathLabel -x 10 -y 40

    place .globLabel -x 10 -y 70 -width 50
    place .fileBrowseEntry -x 65 -y 70 -width 95

    place .fileBrowseList -x 10 -y 100 -height 200 -width 220
    place .fileBrowseScroll -x 230 -y 100 -height 200

    bind .fileBrowseList <Double-Button-1> {foreach i [selection get] {BrowseDirectory $i}}
    bind .fileBrowseEntry <space> { nothing }
    bind .fileBrowseEntry <Return> { BrowseDirectory "./" }
    bind .fileBrowseEntry <Tab> { nothing }

    .fileBrowseEntry insert 0 "*.ppm"

    button .addButton -text "Add" -relief raised -command { AddSelections }
    place .addButton -x 30 -y 310 -width 50

    button .addAllButton -text "Add All" -relief raised -command { AddAllSelections }
    place .addAllButton -x 90 -y 310 -width 100

    ListDirectory

    scale .gopScale -orient horizontal -label "GOP size" -from 1 -to 99
    place .gopScale -x 10 -y 340 -width 200
    .gopScale set 30


    scale .spfScale -orient horizontal -label "Slices per Frame" -from 1 -to 240
    place .spfScale -x 10 -y 405 -width 200
    .spfScale set 1
}

	###############
	# INPUT FILES #
	###############

proc GetInputFiles {} \
{
    set numInput [.fileInputList size]

    if { $numInput == 0 } then \
	{ puts stdout "No Input Files Specified"; return }

    for {set index 0} {$index < $numInput} {set index [expr $index+1]} \
    {
	set inputFile [.fileInputList get $index]
	NextInputFile $inputFile
    }
}


proc AddInputFile {fileName} \
{
    if { [string last "/" $fileName] == -1 } then \
        {.fileInputList insert end $fileName}
}

proc DeleteInputFile {} \
{
    set selections [.fileInputList curselection]

    foreach index $selections {.fileInputList delete $index $index}
}

proc RemoveSelections {} \
{
    set selections [.fileInputList curselection]

    set firstSel [lindex $selections 0]
    set length [llength $selections]
    set lastIndex [expr $length-1]
    set lastSel [lindex $selections $lastIndex]

    .fileInputList delete $firstSel $lastSel
}

proc RemoveAllSelections {} \
{
    .fileInputList delete 0 end
}

proc FileInterface {} \
{
    label .outputFileLabel -text "Output:"
    entry .outputFileEntry
    place .outputFileLabel -x 255 -y 70 -width 50
    place .outputFileEntry -x 310 -y 70 -width 150

    .outputFileEntry insert 0 "output.mpg"
    bind .outputFileEntry <space> { nothing }
    bind .outputFileEntry <Return> { nothing }
    bind .outputFileEntry <Tab> { nothing }

    label .inputFileLabel -text "Input Files:"

    scrollbar .fileInputScroll -command ".fileInputList yview"
    listbox .fileInputList -yscroll ".fileInputScroll set"

    place .inputFileLabel -x 255 -y 100

    place .fileInputList -x 255 -y 130 -width 220 -height 170
    place .fileInputScroll -x 475 -y 130 -height 170

#   bind .fileInputList <Double-Button-1> {foreach i [selection get] {DeleteInputFile $i}}
    bind .fileInputList <Double-Button-1> {DeleteInputFile}

    button .removeButton -text "Remove" -relief raised -command { RemoveSelections }
    place .removeButton -x 255 -y 310 -width 80

    button .removeAllButton -text "Remove All" -relief raised -command { RemoveAllSelections }
    place .removeAllButton -x 340 -y 310 -width 110

    label .convertLabel -text "Convert:"
    entry .convertEntry
    place .convertLabel -x 255 -y 340 -width 85
    place .convertEntry -x 350 -y 340 -width 100

    .convertEntry insert 0 "*"
    bind .convertEntry <space> { nothing }
    bind .convertEntry <Return> { nothing }
    bind .convertEntry <Tab> { nothing }

    label .formatLabel -text "Input Format:"
    place .formatLabel -x 255 -y 370

    radiobutton .yuvButton -text "YUV" -variable formatVar
    radiobutton .ppmButton -text "PPM" -variable formatVar
    radiobutton .pnmButton -text "PNM" -variable formatVar

    place .yuvButton -x 255 -y 400
    place .ppmButton -x 330 -y 400
    place .pnmButton -x 405 -y 400

    .ppmButton select

    label .widthLabel -text "Width:"
    entry .widthEntry
    place .widthLabel -x 255 -y 435 -width 55
    place .widthEntry -x 315 -y 435 -width 45

    .widthEntry insert 0 "352"
    bind .widthEntry <space> { nothing }
    bind .widthEntry <Return> { nothing }
    bind .widthEntry <Tab> { nothing }

    label .heightLabel -text "Height:"
    entry .heightEntry
    place .heightLabel -x 370 -y 435 -width 60
    place .heightEntry -x 435 -y 435 -width 45

    .heightEntry insert 0 "240"
    bind .heightEntry <space> { nothing }
    bind .heightEntry <Return> { nothing }
    bind .heightEntry <Tab> { nothing }
}


	###############
	# OUTPUT FILE #
	###############

proc GetFormat {} \
{
    global formatVar

    if { $formatVar == "yuvButton" } then \
	{ SetFormat YUV } \
    else \
    {
	if { $formatVar == "ppmButton" } then \
	     { SetFormat PPM } \
	else \
	     { SetFormat PNM }
    }
}


proc GetGOPSize {} \
{
    set value [.gopScale get]

    SetGOPSize $value
}


proc GetSPF {} \
{
    set value [.spfScale get]

    SetSPF $value
}


proc GetDimensions {} \
{
    set width [.widthEntry get]
    set height [.heightEntry get]

    SetDimensions $width $height
}


proc GetConversion {} \
{
    set temp [.convertEntry get]
    SetConversion $temp
}


proc GetOutputFileName {} \
{
    set temp [.outputFileEntry get]
    SetOutputFileName $temp
}


proc PrepareToEncode {} \
{
    GetOutputFileName
    GetPattern
    GetInputFiles
    GetPixelType
    GetSearchRange
    GetPSearch
    GetBSearch
    GetQScale
    GetConversion
    GetFormat
    GetGOPSize
    GetSPF
    GetDimensions
}


proc ControlInterface {} \
{
    button .encodeButton -text "Encode" -relief raised -command { PrepareToEncode; encode }
    place .encodeButton -x 40 -y 710 -width 80

    button .playButton -text "Play" -relief raised -command { GetOutputFileName; PlayOutput }
    place .playButton -x 210 -y 710 -width 80

    button .quitButton -text "Quit" -relief raised -command "destroy ."
    place .quitButton -x 380 -y 710 -width 80
}


wm geometry . 500x750

PatternInterface
FileBrowseInterface
FileInterface
MotionVectorSearch
QScale
ControlInterface
