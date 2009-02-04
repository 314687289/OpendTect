dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#Applying Attrinutes to Inline number 320
Comment "-----Applying Attrinutes to Inline number 320------"

TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 320
Input "Crl Start" 600
Input "Crl Stop" 1000
Input "Z Start" 900
Input "Z Stop" 1500
Ok
Button "Cancel"
TreeMenu "Inline`*`*" "Select Attribute`Attributes`CoherencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_coherencyatt_on_inl320.png"
Ok

TreeMenu "Inline`320`CoherencyAttr*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_convolveatt_on_inl320.png"
Ok

TreeMenu "Inline`320`ConvolveAttr*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_curvatureatt_on_inl320.png"
Ok

TreeMenu "Inline`320`CurvatureAttrib" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_dipatt_on_inl320.png"
Ok

TreeMenu "Inline`320`DipAttrib" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_dipangleatt_on_inl320.png"
Ok

TreeMenu "Inline`320`DipAngleAttrib" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_energyatt_on_inl320.png"
Ok

TreeMenu "Inline`320`EnergyAttrib" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_eventatt_on_inl320.png"
Ok

TreeMenu "Inline`320`EventAttrib" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_frequencytatt_on_inl320.png"
Ok

TreeMenu "Inline`320`FrequencyAttrib" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_frequencyfilteratt_on_inl320.png"
Ok


TreeMenu "Inline`320`FrequencyFilterAttrib" "Sel*`Attributes`GapdeconAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$__gapdeconatt_inl320.png"
Ok

TreeMenu "Inline`320`GapdeconAttrib" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_instantaneoustatt_on_inl320.png"
Ok

TreeMenu "Inline`320`InstantaneousAttrib" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_positionatt_on_inl320.png"
Ok

TreeMenu "Inline`320`PositionAttrib" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_referenceshiftatt_on_inl320.png"
Ok

TreeMenu "Inline`320`ReferenceShiftAttrib" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_scalingatt_on_inl320.png"
Ok

TreeMenu "Inline`320`ScalingAttrib" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_similarityatt_on_inl320.png"
Ok

TreeMenu "Inline`320`SimilarityAttrib" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_spectraldecompatt_on_inl320.png"
Ok


TreeMenu "Inline`320`SpectralDecomp*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_velocityfanfilteratt_on_inl320.png"
Ok

TreeMenu "Inline`320`VelocityFanFilter*" "Sel*`Attributes`VolumeStatisticsAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_volumestatisticsatt_on_inl320.png"
Ok

TreeMenu "Inline`320" "Remove"

End
