#include "SceneState.h"

void saveWorldSettings(FILE *F) {
	/* worldSettings windowTitle "Sprint 2: Simple Scene Graph (Parent-Child Transforms)" {
	width 800
	height 600
	spp 4
	backgroundColor [0.5 0.5 0.8]
	backgroundMusic "aryx.s3m"
	debugFont "ExportedFont.png"
	fontTexNumRows 8
	fontTexNumCols 8
	}
	*/
	fprintf(F, "worldSettings windowTitle \"%s\" {\n", gWindowTitle.c_str());
	fprintf(F, "\twidth %i\n", gWidth);
	fprintf(F, "\theight %i\n", gHeight);
	fprintf(F, "\tspp %i\n", gSPP);
	fprintf(F, "\tbackgroundColor [%f %f %f]\n", backgroundColor.r, backgroundColor.g, backgroundColor.b);
	if (gBackgroundMusic != nullptr && gBackgroundMusic->getSoundSource() != 0) fprintf(F, "\tbackgroundMusic \"%s\"\n", gBackgroundMusic->getSoundSource()->getName());
#ifdef _DEBUG
	else ERROR("\tWarning: no background music was found or getSoundSource() returned 0.");
	cout << "\tSkipping debug font settings.\n";
#endif
	fprintf(F, "}\n");
}