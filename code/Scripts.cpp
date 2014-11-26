#include "Scripts.h"

bool MoverScript::setProperty(const string& propertyName, const string& propertyVal) {
	if (propertyName == "velocity") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &velocity.x, &velocity.y, &velocity.z);
	if (propertyName == "minSpeed") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &minSpeed.x, &minSpeed.y, &minSpeed.z);
}
void MoverScript::update(Camera& cam, double dt) {
	assert(node != nullptr);
	if (gWindow == nullptr) return;
	if (glfwGetKey(gWindow, 'A')) node->addTranslation(glm::vec3(-dt*velocity.x, 0, 0));
	if (glfwGetKey(gWindow, 'D')) node->addTranslation(glm::vec3(dt*velocity.x, 0, 0));
	if (glfwGetKey(gWindow, 'W')) node->addTranslation(glm::vec3(0, 0, -dt*velocity.z));
	if (glfwGetKey(gWindow, 'S')) node->addTranslation(glm::vec3(0, 0, dt*velocity.z));
	if (glfwGetKey(gWindow, 'Q')) node->addTranslation(glm::vec3(0, -dt*velocity.y, 0));
	if (glfwGetKey(gWindow, 'E')) node->addTranslation(glm::vec3(0, dt*velocity.y, 0));
	if (glfwGetKey(gWindow, '=')) velocity += minSpeed;
	if (glfwGetKey(gWindow, '-')) velocity -= minSpeed;
	//if (glfwGetKey(gWindow, GLFW_KEY_LEFT)) cam.rotateGlobal(glm::vec3(0, 1, 0), rAmt);
	//if (glfwGetKey(gWindow, GLFW_KEY_RIGHT)) cam.rotateGlobal(glm::vec3(0, 1, 0), -rAmt);
	//if (glfwGetKey(gWindow, GLFW_KEY_UP)) cam.rotateLocal(glm::vec3(1, 0, 0), rAmt);
	//if (glfwGetKey(gWindow, GLFW_KEY_DOWN)) cam.rotateLocal(glm::vec3(1, 0, 0), -rAmt);
	node->T.refreshTransform();
}
void MoverScript::toSDL(FILE *F, const char* tabs) {
	/*
	script {
		type "moverScript"
		pairs {
			velocity [1,1,1]
			minSpeed [1,1,1]
		}
	}
	*/
	fprintf(F, "%sscript {\n", tabs);
	fprintf(F, "\t%stype \"%s\"\n", tabs, type.c_str());
	fprintf(F, "\t%spairs {\n", tabs);
	fprintf(F, "\t\t%svelocity [%f, %f, %f]\n", tabs, velocity.x, velocity.y, velocity.z);
	fprintf(F, "\t\t%sminSpeed [%f, %f, %f]\n", tabs, minSpeed.x, minSpeed.y, minSpeed.z);
	fprintf(F, "\t%s}\n", tabs);
	fprintf(F, "%s}\n", tabs);
}

EmitterScript::EmitterScript(SceneGraphNode *n) : Script(n) { 
	type = "emitterScript"; 
	active = true; 
	posOffset = rotOffset = avgVelocity = glm::vec3(0);
	particleMax = emitRate = 0; 
	currAccumulatedTime = 0.0f; 
	if (gMeshes.count("flatCard") > 0) p.card.setMesh(gMeshes["flatCard"]);
	else if (n != nullptr) ERROR("Unable to locate gMeshes[\"flatCard\"], check scene and library files?", false);
	if (gMaterials.count("allAxes") > 0) p.card.setMaterial(gMaterials["allAxes"]);
	else if (n != nullptr) ERROR("Unable to locate gMaterials[allAxes], check scene and library files?", false);
	p.card.diffuseTexture = nullptr;
}
bool EmitterScript::setProperty(const string& propertyName, const string& propertyVal) {
	if (propertyName == "avgVelocity") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &avgVelocity.x, &avgVelocity.y, &avgVelocity.z);
	if (propertyName == "rotOffset") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &rotOffset.x, &rotOffset.y, &rotOffset.z);
	if (propertyName == "posOffset") return sscanf(propertyVal.c_str(), "[%f,%f,%f]", &posOffset.x, &posOffset.y, &posOffset.z);
	if (propertyName == "timeToLive") return sscanf(propertyVal.c_str(), "%f",  &p.timeToLive);
	if (propertyName == "active") return sscanf(propertyVal.c_str(), "%d", &active);
	if (propertyName == "particleMax") return sscanf(propertyVal.c_str(), "%d", &particleMax);
	if (propertyName == "emitRate") return sscanf(propertyVal.c_str(), "%f", &emitRate);
	if (propertyName == "image") {
		p.card.diffuseTexture = new RGBAImage();
		p.card.diffuseTexture->name = "uDiffuseTex";
		p.card.diffuseTexture->fileName = propertyVal;
		bool v = p.card.diffuseTexture->loadPNG(p.card.diffuseTexture->fileName);
		p.card.diffuseTexture->sendToOpenGL();
		return v;
	}
}
void EmitterScript::update(Camera& cam, double dt) {
	//Spawn by adding to particle list if enough time has passed in lieu of sprite ticking.
	if (currAccumulatedTime >= emitRate) {
		if (particles.size() < particleMax) {
			Particle * _p = new Particle();
			_p->T.translation = posOffset + node->T.translation; //Add a rand().
			_p->T.rotation = glm::quat(rotOffset); //Add a rand().
			_p->T.scale = glm::vec3(1);
			_p->velocity = avgVelocity; //Add a rand().
			_p->card = p.card;
			_p->timeToLive = p.timeToLive;
			particles.push_back(_p);
		}
		currAccumulatedTime = 0.0f;
	}
	else currAccumulatedTime += dt;

	//Iterate over all particles in the list, remove those past TTL, else add velocity, and then render.
	for (auto it = particles.begin(); it != particles.end(); ++it) {

		//Might add a line here to effectively repeat node::addTranslation().
		(*it)->T.refreshTransform();
		(*it)->card.prepareToDraw(cam, (*it)->T, *p.card.material);

		glUseProgram(p.card.material->shaderProgramHandles[p.card.material->activeShaderProgram]);

		GLint loc;

		loc = glGetUniformLocation(p.card.material->shaderProgramHandles[p.card.material->activeShaderProgram], "uObjectWorldM");
		if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr((*it)->T.transform));
#ifdef _DEBUG
		//else ERROR("Could not load uniform uObjectWorldM.", false);
#endif

		loc = glGetUniformLocation(p.card.material->shaderProgramHandles[p.card.material->activeShaderProgram], "uObjectWorldInverseM");
		if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr((*it)->T.invTransform));
#ifdef _DEBUG
		//else ERROR("Could not load uniform uObjectWorldInverseM.", false);
#endif

		glm::mat4x4 objectWorldViewPerspect = cam.worldViewProject * (*it)->T.transform;
		loc = glGetUniformLocation(p.card.material->shaderProgramHandles[p.card.material->activeShaderProgram], "uObjectPerpsectM");
		if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(objectWorldViewPerspect));
#ifdef _DEBUG
		//else ERROR("Could not load uniform uObjectPerpsectM.", false);
#endif

		loc = glGetUniformLocation(p.card.material->shaderProgramHandles[p.card.material->activeShaderProgram], "uViewDirection");
		if (loc != -1) glUniform4fv(loc, 1, glm::value_ptr(cam.center));
#ifdef _DEBUG
		//else ERROR("Could not load uniform uViewPosition.", false);
#endif

		loc = glGetUniformLocation(p.card.material->shaderProgramHandles[p.card.material->activeShaderProgram], "uViewPosition");
		if (loc != -1) glUniform4fv(loc, 1, glm::value_ptr(cam.eye));
#ifdef _DEBUG
		//else ERROR("Could not load uniform uViewDirection.", false);
#endif
		(*it)->card.draw(cam);

		glUseProgram(0);

		if ((*it)->timeToLive <= 0) it = particles.erase(it);
		else {
			(*it)->timeToLive -= dt;
			(*it)->T.translation += glm::vec3(dt) * (*it)->velocity;
		}
	}

}
void EmitterScript::toSDL(FILE *F, const char* tabs) {
	/*
	script {
		type "moverScript"
		pairs {
			velocity [1,1,1]
			minSpeed [1,1,1]
		}
	}
	*/
	fprintf(F, "%sscript {\n", tabs);
	fprintf(F, "\t%stype \"%s\"\n", tabs, type.c_str());
	fprintf(F, "\t%spairs {\n", tabs);
	fprintf(F, "\t\t%savgVelocity [%f, %f, %f]\n", tabs, avgVelocity.x, avgVelocity.y, avgVelocity.z);
	fprintf(F, "\t\t%srotOffset [%f, %f, %f]\n", tabs, rotOffset.x, rotOffset.y, rotOffset.z);
	fprintf(F, "\t\t%sposOffset [%f, %f, %f]\n", tabs, posOffset.x, posOffset.y, posOffset.z);
	fprintf(F, "\t\t%stimeToLive %f\n", tabs, p.timeToLive);
	fprintf(F, "\t\t%sparticleMax %d\n", tabs, particleMax);
	fprintf(F, "\t\t%semitRate %f\n", tabs, emitRate);
	fprintf(F, "\t\t%simage \"%s\"\n", tabs, p.card.diffuseTexture->fileName.c_str());
	fprintf(F, "\t%s}\n", tabs);
	fprintf(F, "%s}\n", tabs);
}

RGBGameScript::RGBGameScript(SceneGraphNode *n) : Script(n) {
	type = "rgbGameScript";
	active = true;
	hit = block = false;
	order = glm::vec4(0);
	srand((unsigned)time(NULL));
	if (gNodes.count("N") < 1 || gNodes.count("S") < 1 || gNodes.count("E") < 1 || gNodes.count("W") < 1 ||
		gNodes.count("Player") < 1 || gNodes.count("WinText") < 1 || gNodes.count("LossText") < 1) {
		if (n != nullptr) ERROR("FAILURE LOADING NODES, CHECK SDL!");
		else return;
	}
	N = gNodes["N"];
	S = gNodes["S"];
	E = gNodes["E"];
	W = gNodes["W"];
	P = gNodes["Player"];
	currTarget = nullptr;
	WinText = gNodes["WinText"]; WinText->isRendered = false;
	LossText = gNodes["LossText"]; LossText->isRendered = false;
	if (gBackgroundMusic) gBackgroundMusic->setIsPaused(false);
}
void RGBGameScript::postParseInit() {
	for (int i = 1; i <= maxEnemyHP; ++i) {
		N->children.push_back(gNodes["NH" + to_string(i)]);
		S->children.push_back(gNodes["SH" + to_string(i)]);
		E->children.push_back(gNodes["EH" + to_string(i)]);
		W->children.push_back(gNodes["WH" + to_string(i)]);
		if (i > initEnemyHP) {
			N->children.back()->isRendered = false;
			S->children.back()->isRendered = false;
			E->children.back()->isRendered = false;
			W->children.back()->isRendered = false;
		}
	}
	for (int i = 1; i <= maxPlayerHP; ++i) {
		P->children.push_back(gNodes["PH" + to_string(i)]);
		if (i > initPlayerHP) P->children.back()->isRendered = false;
	}
	currMaxPlayerHP = initPlayerHP;
}
bool RGBGameScript::setProperty(const string& propertyName, const string& propertyVal) {
	if (propertyName == "order") return sscanf(propertyVal.c_str(), "[%f,%f,%f,%f]", &order.x, &order.y, &order.z, &order.w);
	if (propertyName == "enemyTickRate") return sscanf(propertyVal.c_str(), "%f", &enemyTickRate);
	if (propertyName == "initPlayerHP") return sscanf(propertyVal.c_str(), "%d", &initPlayerHP);
	if (propertyName == "initEnemyHP") return sscanf(propertyVal.c_str(), "%d", &initEnemyHP);
	if (propertyName == "maxPlayerHP") return sscanf(propertyVal.c_str(), "%d", &maxPlayerHP);
	if (propertyName == "maxEnemyHP") return sscanf(propertyVal.c_str(), "%d", &maxEnemyHP);
	if (propertyName == "winSound") winSound = soundEngine->play2D(propertyVal.c_str(), false, true, true, irrklang::ESM_AUTO_DETECT, true);
	if (propertyName == "lossSound") lossSound = soundEngine->play2D(propertyVal.c_str(), false, true, true, irrklang::ESM_AUTO_DETECT, true);
	if (propertyName == "deathSound") deathSound = soundEngine->play2D(propertyVal.c_str(), false, true, true, irrklang::ESM_AUTO_DETECT, true);
	if (propertyName == "hitSound") hitSound = soundEngine->play2D(propertyVal.c_str(), false, true, true, irrklang::ESM_AUTO_DETECT, true);
	if (propertyName == "enemySound") enemySound = soundEngine->play2D(propertyVal.c_str(), false, true, true, irrklang::ESM_AUTO_DETECT, true);
	return true;
}
SceneGraphNode* RGBGameScript::getAttackerFromInt(int id) {
	switch (id) {
	case Enemy::NORTH: return N;
	case Enemy::SOUTH: return S;
	case Enemy::EAST: return E;
	case Enemy::WEST: return W;
	}
	return P;
}
void RGBGameScript::update(Camera& cam, double dt) {
	//Enemy loop.
	if (accTime + dt >= enemyTickRate) {
		if (cooldown == false && accEnemyTicks > 0) hurtPlayer(); //Player was able to attack but didn't respond in time!
		cooldown = false; //Player can now attack again if they weren't yet able to.	
		//P->LODstack[P->activeLOD]->getMaterial()->colors[0]->val.r += colorThreshold;
		//Generate a random agent if there is no order (i.e. order == glm::vec4(0)).
		do {
			currAttackerId = (order == glm::vec4(0)) ? (rand() % 4) + 1 : getNextEnemyFromOrder();
			currAttacker = getAttackerFromInt(currAttackerId);
		} while (currAttacker == nullptr || !currAttacker->isRendered);
		soundEngine->play2D(enemySound->getSoundSource());
		cout << endl << currAttacker->name << " is attacking!\n";
		switch ((rand() % 3) + 1) {
		case Color::R: if (currAttacker->LODstack[currAttacker->activeLOD]->getMaterial()->colors[0]->val.r <= colorThreshold) 
			currAttacker->LODstack[currAttacker->activeLOD]->getMaterial()->colors[0]->val.r += colorThreshold; break;
		case Color::G: if (currAttacker->LODstack[currAttacker->activeLOD]->getMaterial()->colors[0]->val.g <= colorThreshold) 
			currAttacker->LODstack[currAttacker->activeLOD]->getMaterial()->colors[0]->val.g += colorThreshold; break;
		case Color::B: if (currAttacker->LODstack[currAttacker->activeLOD]->getMaterial()->colors[0]->val.b <= colorThreshold) 
			currAttacker->LODstack[currAttacker->activeLOD]->getMaterial()->colors[0]->val.b += colorThreshold; break;
		}
		accTime = 0.0;
		accEnemyTicks++;
		return;
	}
	
	//Will not reach this code if ticking an enemy, so we can take player input here.
	if (!cooldown) {
		if (glfwGetKey(gWindow, GLFW_KEY_UP)) currTarget = N;
		else if (glfwGetKey(gWindow, GLFW_KEY_DOWN)) currTarget = S;
		else if (glfwGetKey(gWindow, GLFW_KEY_RIGHT)) currTarget = E;
		else if (glfwGetKey(gWindow, GLFW_KEY_LEFT)) currTarget = W;
		else currTarget = nullptr;
	}

	//Hit/Block/Hurt logic.
	if (currTarget != nullptr) {
		cooldown = true;
		hit = block = false;
		counterR = glfwGetKey(gWindow, 'R');
		counterG = glfwGetKey(gWindow, 'G');
		counterB = glfwGetKey(gWindow, 'B');
		counteredR = currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.r > colorThreshold;
		counteredG = currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.g > colorThreshold;
		counteredB = currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.b > colorThreshold;

		if ((counterR && counteredR) || (counterG && counteredG) || (counterB && counteredB)) hit = true;

		/* //Killed by GLFW not recording more than one among R, G, and B keys being pressed at one time. :((((
		if (counterR && !counterG && !counterB) hit = counteredR; 
		else if (!counterR && counterG && !counterB) hit = counteredG; 
		else if (!counterR && !counterG && counterB) hit = counteredB; 
		else if (counterR && counterG && !counterB) { hit = counteredR && counteredG; }
		else if (counterR && !counterG && counterB) { hit = counteredR && counteredB; }
		else if (!counterR && counterG && counterB) { hit = counteredG && counteredB; }
		else if (counterR && counterG && counterB) { hit = counteredR && counteredG && counteredB; }
		*/
		if (!(counterR || counterG || counterB)) block = true; //The all-false case from a no-reply player.
		//Originally allowed 2 enemies to attack at once--block one, hit another--but above else-if's on the arrow keys prevent this (if they didn't, a press-all-keys would auto-win).
		
		if (hit) {
			cout << "Player landed a hit!\n";
			if (currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.r > colorThreshold) currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.r -= colorThreshold;
			if (currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.g > colorThreshold) currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.g -= colorThreshold;
			if (currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.b > colorThreshold) currTarget->LODstack[currTarget->activeLOD]->getMaterial()->colors[0]->val.b -= colorThreshold;
			for (int i = 0; i < currTarget->children.size(); ++i)
				if (currTarget->children[i]->isRendered) {
					currTarget->children[i]->isRendered = false; //Kill heart.
					if (i == currTarget->children.size() - 1) { //Last heart died.
						healPlayer(); //Player takes a heart on enemy death, heal sound plays, BG brightens slightly.
						if (deathSound != nullptr) soundEngine->play3D(deathSound->getSoundSource(), irrklang::vec3df(currTarget->T.translation.x, currTarget->T.translation.y, currTarget->T.translation.z));
						else ERROR("DEATH SOUND NOT LOADED");
						currTarget->isRendered = false; //RIP enemy.
						break;
					}
					if (hitSound != nullptr) soundEngine->play3D(hitSound->getSoundSource(), irrklang::vec3df(currTarget->T.translation.x, currTarget->T.translation.y, currTarget->T.translation.z));
					else ERROR("HIT SOUND NOT LOADED");
					break;
			}
			hit = false;
		}
		else if (!block) {
			cout << "Player missed a hit and took damage!\n";
			hurtPlayer(); 
			block = false; 
		} //Player hurt when hit is false (missed attack) and block is false (so an attack R, G, or B was attempted).
		currTarget = nullptr;
	}

	//Win-Loss check.
	if (!(N->isRendered || S->isRendered || E->isRendered || W->isRendered)) {
		WinText->isRendered = true;
		if (winSound != nullptr) {
			soundEngine->stopAllSounds();
			soundEngine->play2D(winSound->getSoundSource());
		}
		else ERROR("WIN SOUND NOT LOADED");
		this->active = false; //Turn script off.
	}
	else if (!P->isRendered) {
		LossText->isRendered = true;
		if (winSound != nullptr) {
			soundEngine->stopAllSounds();
			soundEngine->play2D(lossSound->getSoundSource());
		}
		else ERROR("LOSS SOUND NOT LOADED");
		this->active = false; //Turn script off.
	}
	accTime += dt;
}
int RGBGameScript::getNextEnemyFromOrder() {
	if (!(currAttackerId == order.x || currAttackerId == order.y || currAttackerId == order.z || currAttackerId == order.w)) return order.x;
	if (currAttackerId == order.x) return order.y;
	if (currAttackerId == order.y) return order.z;
	if (currAttackerId == order.z) return order.w;
	if (currAttackerId == order.w) return order.x;
}
void RGBGameScript::healPlayer() {
	if (currMaxPlayerHP == maxPlayerHP) return;
	for (int i = 0; i < P->children.size(); ++i) { //In general, we heal the first heart we find from [0].
		if ((i < initPlayerHP && !P->children[i]->isRendered && P->children[i + 1]->isRendered) || (i >= initPlayerHP && !P->children[i]->isRendered)) {
			//The i+1 condition is to try and keep us from having x o x o x isolated active heart patterns--we want all active hearts to be consecutive.
			P->children[i]->isRendered = true;
			currMaxPlayerHP++;
			//Case 1 heals initially active hearts || case 2 activates new hearts on top of that. 
			gBackgroundColor.r += bgColorChangeAmt;
			gBackgroundColor.g += bgColorChangeAmt;
			gBackgroundColor.b += bgColorChangeAmt;
			//No heal sound because the enemy's death sound is already playing when we get healed.
			return;
		}
	}
}
void RGBGameScript::hurtPlayer() {
	for (int i = 0; i < P->children.size(); ++i) {
		if (P->children[i]->isRendered) {
			P->children[i]->isRendered = false; //Kill heart.
			gBackgroundColor.r -= bgColorChangeAmt;
			gBackgroundColor.g -= bgColorChangeAmt;
			gBackgroundColor.b -= bgColorChangeAmt;
			if (hitSound != nullptr) soundEngine->play3D(hitSound->getSoundSource(), irrklang::vec3df(P->T.translation.x, P->T.translation.y, P->T.translation.z));
			else ERROR("HIT SOUND NOT LOADED");
			return;
		}
	}
	//No hearts were rendered, RIP player.
	if (deathSound != nullptr) soundEngine->play3D(deathSound->getSoundSource(), irrklang::vec3df(P->T.translation.x, P->T.translation.y, P->T.translation.z));
	else ERROR("DEATH SOUND NOT LOADED");
	P->isRendered = false;
	return;
}
void RGBGameScript::toSDL(FILE *F, const char* tabs) {
	/*
	script {
		type "rgbGameScript"
		pairs {
			order [4, 2, 3, 1] //glm::vec4 order is iterated over by enemy tick code, switching on val of x, then y, z, w.
			enemyTickRate 2.0
			initPlayerHP 3
			initEnemyHP 3
			maxPlayerHP 5
			maxEnemyHP 3
			winSound "win.mp3"
			lossSound "loss.mp3"
			deathSound "death.mp3"
			hitSound "hit.mp3"
		}
	}
	*/
	fprintf(F, "%sscript {\n", tabs);
	fprintf(F, "\t%stype \"%s\"\n", tabs, type.c_str());
	fprintf(F, "\t%spairs {\n", tabs);
	fprintf(F, "\t\t%sorder [%f, %f, %f, %f]\n", tabs, order.x, order.y, order.z, order.w);
	fprintf(F, "\t\t%senemyTickRate %f\n", tabs, enemyTickRate);
	fprintf(F, "\t\t%sinitPlayerHP %d\n", tabs, initPlayerHP);
	fprintf(F, "\t\t%sinitEnemyHP %d\n", tabs, initEnemyHP);
	fprintf(F, "\t\t%smaxPlayerHP %d\n", tabs, maxPlayerHP);
	fprintf(F, "\t\t%smaxEnemyHP %d\n", tabs, maxEnemyHP);
	fprintf(F, "\t\t%swinSound \"%s\"\n", tabs, winSound->getSoundSource()->getName());
	fprintf(F, "\t\t%slossSound \"%s\"\n", tabs, lossSound->getSoundSource()->getName());
	fprintf(F, "\t\t%sdeathSound \"%s\"\n", tabs, deathSound->getSoundSource()->getName());
	fprintf(F, "\t\t%shitSound \"%s\"\n", tabs, hitSound->getSoundSource()->getName());
	fprintf(F, "\t\t%senemySound \"%s\"\n", tabs, enemySound->getSoundSource()->getName());
	fprintf(F, "\t%s}\n", tabs);
	fprintf(F, "%s}\n", tabs);
}