#include "gui.h"

using namespace ImGui;
using namespace guiVariables;

//taken from imgui_demo, copy past is allowed
// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
	TextDisabled("(?)");
	if (IsItemHovered())
	{
		BeginTooltip();
		PushTextWrapPos(GetFontSize() * 35.0f);
		TextUnformatted(desc);
		PopTextWrapPos();
		EndTooltip();
	}
}

void GUI::setScaling(float scaleFactor) {
	//reset to default scaling
	if (prevScaleFactor != 0) {
		GetStyle().ScaleAllSizes(1.0f / scaleFactor);
	}

	GetIO().Fonts->Clear();
	GetIO().Fonts->AddFontFromFileTTF("resources/font/OpenSans-Regular.ttf", (float)(constants::FONT_SIZE * scaleFactor));
	ImGui_ImplOpenGL3_CreateFontsTexture();
	GetStyle().ScaleAllSizes(scaleFactor);
	prevScaleFactor = scaleFactor;
}

bool GUI::hasMouse() {
	return GetIO().WantCaptureMouse;
}

void GUI::initialise(GLFWwindow* _window, ParticleSystem* _particleSystem) {
	window = _window;
	particleSystem = _particleSystem;

	IMGUI_CHECKVERSION();
	CreateContext();

	StyleColorsDark();
	GetStyle().FrameRounding = 4.0f;
	GetStyle().IndentSpacing = 6.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	fluidNames[0] = "Water";
	fluidNames[1] = "Oil";
	for (int i = 2; i < fluidNames.size(); i++) {
		fluidNames[i] = "Unnamed Fluid";
	}
}

void GUI::update() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	NewFrame();

	//bool showDemoWindow = true;
	//ShowDemoWindow(&showDemoWindow);

	Begin("Tool Box", NULL, ImGuiWindowFlags_NoCollapse);

	ImGuiIO& io = GetIO();

	// Basic info
	Text("Simulation average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	Text("Particles: %d", particleSystem->getParticles());

	//edit mode
	if (CollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		RadioButton("Fluid", &editMode, 0); SameLine();
		HelpMarker("Hold left clicked to spawn selected fluid (hold shift to speed it up).\nHold right click to remove fluid."); SameLine();
		RadioButton("Geometry", &editMode, 1); SameLine();
		HelpMarker("Left click to start creation a line, right click to cancel.\nRight click existing line to delete.\nLines snap together during creation at each end point.");

		Separator();

		if (editMode == 0) {
			//show options for fluid creation
			Text("Control Settings");
			DragInt("Spawn Speed", &spawnSpeed, 0.05f, 0.1f, 10.0f, "%d Particles", ImGuiSliderFlags_AlwaysClamp);
			DragFloat("Spawn Radius", &spawnRadius, 0.05f, 0.1f, 100.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp);
			DragFloat("Delete Radius", &deleteRadius, 0.05f, 0.1f, 100.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp);

			if (CollapsingHeader("Spawn Dam", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();
				InputFloat2("Location", &damPosition[0]); SameLine();
				HelpMarker("Centre is (640, 360) Values above (1280, 720) and below (0, 0) may cause issues.");
				DragInt("Size", &damSize, 0.05f, 0.1f, 30.0f, "%d Square of Particles", ImGuiSliderFlags_AlwaysClamp);
				if (Button("Spawn")) {
					particleSystem->spawnDam(damSize, selectedFluid, damPosition.x, damPosition.y);
				}
				Unindent();
			}

			//Text("Fluids");
			if (CollapsingHeader("Fluid Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();
				if (BeginTable("split", 2)) {
					for (int i = 0; i < 6; i++) {
						ParticleProperties pp = particleSystem->particleProperties[i];

						TableNextColumn();
						RadioButton(fluidNames[i].c_str(), &selectedFluid, i);

						TableNextColumn();
						ImVec4 color = ImVec4(pp.color.x, pp.color.y, pp.color.z, pp.color.w);
						ColorEdit4("", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); SameLine();
						std::string fluid = "Edit Fluid " + std::to_string(i);
						if (Button(fluid.c_str())) OpenPopup(std::to_string(i).c_str());

						//popup window for editing fluid properties
						if (BeginPopup(std::to_string(i).c_str()))
						{
							std::string txt = "Edit " + fluidNames[i] + "\'s properties";
							Text(txt.c_str());

							//edit name of fluid
							static char str0[32] = "";
							for (int j = 0; j < fluidNames[i].size(); j++) {
								str0[j] = fluidNames[i][j];
							}
							str0[fluidNames[i].size()] = '\0'; //risky ;)
							InputText("Fluid Name", str0, IM_ARRAYSIZE(str0));
							fluidNames[i] = str0;

							bool needToUpdate = false;

							//numeric properties
							if (DragFloat("Mass", &pp.mass, 0.05f, 0.1f, 10.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp)) needToUpdate = true;
							if (DragFloat("Viscosity", &pp.viscosity, 0.05f, 0.1f, 20.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp)) needToUpdate = true;
							pp.restDensity = constants::REST_DENSITY * pp.mass;

							//color
							if (ColorEdit4("Colour", (float*)&color, NULL)) needToUpdate = true;;
							pp.color = glm::vec4(color.x, color.y, color.z, color.w);

							//diseased, conversion to bool OK
							if (Checkbox("Diseased", (bool*)&pp.diseased)) needToUpdate = true;

							//update to gpu if necessary
							if (needToUpdate) {
								particleSystem->particleProperties[i] = pp;
								particleSystem->updateProperties();
							}

							EndPopup();
						}
					}
					EndTable();
				}
				Unindent();
			}
		}
		else {
			//show options for geometry (select curved or line mode)
			Text("Geometry Creation Settings");
		}
		Unindent();
	}

	//vector field controls
	if (CollapsingHeader("Vector Fields", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		bool needToUpdate = false;
		if (Checkbox("Gravity", &particleSystem->gravityEnabled)) needToUpdate = true;
		if (Checkbox("Wind", &particleSystem->windEnabled)) needToUpdate = true;
		Indent();
		if (InputFloat2("Wind Centre", &particleSystem->windCentre[0])) needToUpdate = true;
		if (InputFloat2("Wind Strength", &particleSystem->windStrength[0])) needToUpdate = true;
		Unindent();
		if (needToUpdate) particleSystem->setVectorField();
		Unindent();
	}

	//scene controls (delete all particles, geometry, save, load)
	if (CollapsingHeader("Scene Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		if (Button("Reset Particles")) particleSystem->resetParticles();
		if (Button("Reset Geometry")) particleSystem->resetGeometry();
		if (Button("Save State")) particleSystem->saveState();
		if (Button("Load State")) particleSystem->loadState();
		Unindent();
	}

	//simulation settings
	if (CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();

		//put a reset button here
		if (Button("Reset")) {
			gravity = constants::GRAVITY;
			particleSystem->updateGravity();
		}

		//mouse interaction(blowing) enable
		
		//InputFloat("Gravity", &gravity, 0.0f, 0.0f, "%2.2f m/s^2"); SameLine();
		//SliderFloat("Gravity", &gravity, -10.0f, 10.0f, "%2.2f m/s^2");
		if (DragFloat("Gravity", &gravity, 0.05f, -10.0f, 10.0f, "%2.2f m/s^2", ImGuiSliderFlags_AlwaysClamp)) {
			particleSystem->updateGravity();
		}
		
		SameLine();
		HelpMarker("You can click and drag the value,\nor double click to type in your own.");

		//change geometry and bounds damping

		//very risky, but allow the user to change DT for slow motion, maybe just have toggles for 1.0x 0.5x 0.25x?

		Unindent();
	}

	//particle types

	//visual settings (colors of geometry and background)

	//simulation metrics

	//SameLine(); HelpMarker("Choose what will be created (and destroyed) when you click inside the simulation");
	End();
}

void GUI::render() {
	Render();
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
}