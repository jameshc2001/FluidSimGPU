#include "gui.h"

using namespace ImGui;
using namespace guiVariables;

//code here is taken from https://eliasdaler.github.io/using-imgui-with-sfml-pt2/#arrays
namespace ImGui
{
	static auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return Combo(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}

	bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return ListBox(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}

}

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

void GUI::updateFileNames() {
	fileNames.clear();
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		fileNames.push_back(entry.path().u8string().erase(0,10));
	}
}

void GUI::initialise(GLFWwindow* _window, ParticleSystem* _particleSystem) {
	window = _window;
	particleSystem = _particleSystem;
	blower = &particleSystem->blower;

	IMGUI_CHECKVERSION();
	CreateContext();

	StyleColorsDark();
	GetStyle().FrameRounding = 4.0f;
	GetStyle().IndentSpacing = 6.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	//fluidNames[0] = "Water";
	//fluidNames[1] = "Oil";
	//for (int i = 2; i < fluidNames.size(); i++) {
	//	fluidNames[i] = "Unnamed Fluid";
	//}

	//savedFluidNames = fluidNames;

	updateFileNames();
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
	Text("Particles: %d, \t Diseased: %d", particleSystem->getParticles(), particleSystem->getDiseased());

	//edit mode
	if (CollapsingHeader("Edit Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		RadioButton("Fluid", &editMode, 0); SameLine();
		HelpMarker("Hold left click to spawn selected fluid.\nHold right click to remove fluid."); SameLine();
		RadioButton("Geometry", &editMode, 1); SameLine();
		HelpMarker("Left click to start creating a line, right click to cancel.\nRight click existing line to delete.\nLines snap together during creation at each end point."); SameLine();
		RadioButton("Blower", &editMode, 2); SameLine();
		HelpMarker("Left click to place blower.\nRight click to select direction and length.");

		Separator();

		if (editMode == 0) {
			//show options for fluid creation
			Text("Control Settings");
			DragInt("Spawn Speed", &spawnSpeed, 0.05f, 1, 10, "%d Particles", ImGuiSliderFlags_AlwaysClamp);
			DragFloat("Spawn Interval", &spawnInterval, 0.05f, 0.01f, 1.0f, "%2.2f Seconds", ImGuiSliderFlags_AlwaysClamp);
			DragFloat("Spawn Radius", &spawnRadius, 0.05f, 0.1f, 100.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp);
			DragFloat("Delete Radius", &deleteRadius, 0.05f, 0.1f, 100.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp);

			if (CollapsingHeader("Spawn Dam", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();
				InputFloat2("Location", &damPosition[0]); SameLine();
				HelpMarker("Centre is (640, 360) Values above (1280, 720) and below (0, 0) may cause issues.");
				DragInt("Size", &damSize, 0.05f, 1, 30, "%d Square of Particles", ImGuiSliderFlags_AlwaysClamp);
				if (Button("Spawn")) {
					particleSystem->spawnDam(damSize, selectedFluid, damPosition.x, damPosition.y);
				}
				Unindent();
			}

			//Text("Fluids");
			if (CollapsingHeader("Fluid Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();
				std::array<std::string, MAX_PARTICLE_TYPES>& fluidNames = particleSystem->fluidNames; //nicer way to do this?
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
							Text(txt.c_str()); SameLine();
							const char* warning = "High mass and high viscosities can cause chaos, experiment to find ideal values.\n"
								"Units are not necessarily physically accurate and are mostly included for educational value.";
							HelpMarker(warning);

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
							float displayedMass = pp.mass * 1000.0f;
							if (DragFloat("Density", &displayedMass, 5.0f, 100.0f, 20000.0f, "%2.2f kg/m^3", ImGuiSliderFlags_AlwaysClamp)) {
								pp.mass = displayedMass / 1000.0f;
								needToUpdate = true;
							}
							//if (DragFloat("Density", &pp.mass, 0.05f, 0.01f, 20.0f, "%2.2f * 10^2 kg/m^3", ImGuiSliderFlags_AlwaysClamp)) needToUpdate = true;
							float displayedViscosity = 100.0f * pp.viscosity;
							if (DragFloat("Viscosity", &displayedViscosity, 1.0f, 1.0f, 5000.0f, "%2.2f mPa-s", ImGuiSliderFlags_AlwaysClamp)) {
								pp.viscosity = displayedViscosity / 100.0f;
								needToUpdate = true;
							}
							//if (DragFloat("Viscosity", &pp.viscosity, 0.05f, 0.01f, 100.0f, "%2.2f * 10^2 mPa-s", ImGuiSliderFlags_AlwaysClamp)) needToUpdate = true;
							pp.restDensity = constants::REST_DENSITY * pp.mass;

							if (DragFloat("Stickiness", &pp.stickiness, 0.2f, 0.0f, 200.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp)) { needToUpdate = true;  }

							//color
							if (ColorEdit4("Colour", (float*)&color, NULL)) needToUpdate = true;;
							pp.color = glm::vec4(color.x, color.y, color.z, color.w);

							//diseased, conversion to bool OK
							bool updateDiseased = false;
							/*if (Checkbox("Diseased", (bool*)&pp.diseased)) {
								needToUpdate = true;
								updateDiseased = true;
							}*/
							if (Checkbox("Diseased", (bool*)&particleSystem->diseased[i])) {
								needToUpdate = true;
								updateDiseased = true;
							}

							//warning about properties
							if (pp.mass * pp.viscosity > 25) {
								TextColored(ImVec4(1, 0, 0, 1), "WARNING: Fluid properties may cause chaos,\nhigh values for mass and viscosity are not recommended.");
							}

							//update to gpu if necessary
							if (needToUpdate) {
								particleSystem->particleProperties[i] = pp;
								particleSystem->updateProperties();
							}

							if (updateDiseased) particleSystem->updateDiseased();

							EndPopup();
						}
					}
					EndTable();
				}
				Unindent();
			}
		}
		else if (editMode == 1) {
			//show options for geometry
			Text("Geometry Creation Settings");
			Checkbox("Snap To Vertex", &lineSnap); SameLine();
			HelpMarker("When creating a new line, if this setting is enabled, the start of the new line will snap to a nearby existing line vertex.");
		}
		else {
			Text("Blower Settings");

			bool needToUpdate = false;

			if (Checkbox("Visible", &blower->visible)) { needToUpdate = true; }
			if (DragFloat("Source Width", &blower->sourceWidth, 5.0f, 5.0f, 500.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp)) { needToUpdate = true; }
			if (DragFloat("End Width", &blower->endWidth, 5.0f, 5.0f, 500.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp)) { needToUpdate = true; }
			if (DragFloat("Strength", &blower->strength, 0.5f, 0.0f, 20.0f, "%2.2f", ImGuiSliderFlags_AlwaysClamp)) { needToUpdate = true; }
			if (Checkbox("On", &blower->on)) { needToUpdate = true; }

			if (needToUpdate) {
				blower->setupBlower(particleSystem->getPredictShader());
			}
		}
		Unindent();
	}

	//vector field controls
	if (CollapsingHeader("Vector Fields", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		bool needToUpdate = false;
		if (Checkbox("Gravity", &particleSystem->gravityEnabled)) needToUpdate = true;
		Indent();
		if (DragFloat("Gravity Value", &gravity, 0.05f, -10.0f, 10.0f, "%2.2f m/s^2", ImGuiSliderFlags_AlwaysClamp)) {
			particleSystem->updateGravity();
		}
		SameLine();
		HelpMarker("You can click and drag the value,\nor double click to type in your own.");
		Unindent();


		if (Checkbox("Wind", &particleSystem->windEnabled)) needToUpdate = true;
		Indent();
		if (InputFloat2("Wind Centre", &particleSystem->windCentre[0])) needToUpdate = true;
		if (InputFloat2("Wind Strength", &particleSystem->windStrength[0])) needToUpdate = true;
		Unindent();
		if (needToUpdate) particleSystem->setVectorField();
		Unindent();
	}

	//disease controls
	if (CollapsingHeader("Disease", ImGuiTreeNodeFlags_DefaultOpen)) {
		Text("Physical Distrubtion: %2.2f", diseaseDistribution); SameLine();
		HelpMarker("Low values indicate that the diseased particles are located in one area, high values indicate that they are spread apart throughout the scene.");
		if (Button("Update Physical Distribution")) {
			particleSystem->measureDiseaseDistribution();
		}
	}

	//scene controls (delete all particles, geometry, save, load)
	if (CollapsingHeader("Scene Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		if (Button("Reset Particles")) particleSystem->resetParticles();
		if (Button("Reset Geometry")) particleSystem->resetGeometry();
		Unindent();
	}

	//loading saving options
	if (CollapsingHeader("Save and Load", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		if (Button("Quick Save")) particleSystem->saveState(false); SameLine();
		if (Button("Quick Load")) particleSystem->loadState(false);

		Separator();
		
		//save to file
		static char str0[32] = "";
		for (int j = 0; j < newFile.size(); j++) {
			str0[j] = newFile[j];
		}
		str0[newFile .size()] = '\0'; //risky ;)
		InputText("File Name", str0, IM_ARRAYSIZE(str0));
		newFile = str0;

		if (Button("Save State to File")) {
			particleSystem->saveToFile(path + newFile + ".fsg");
			updateFileNames();
		}

		ListBox("Scenarios", &selectedFileName, fileNames);

		if (fileNames.size() > 0 && selectedFileName < fileNames.size()) {
			if (Button("Load from Selected File")) {
				//std::cout << fileNames[selectedFileName] << std::endl;
				particleSystem->loadFromFile(path + fileNames[selectedFileName]);
			}

			if (Button("Delete Selected File")) {
				std::filesystem::remove(path + fileNames[selectedFileName]);
				updateFileNames();
			}
		}

		Unindent();
	}

	//simulation settings
	if (CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();

		//put a reset button here
		//if (Button("Reset")) {
		//	gravity = constants::GRAVITY;
		//	particleSystem->updateGravity();
		//}

		RadioButton("Draw Fluids", &particleSystem->drawMode, 0); SameLine();
		RadioButton("Draw Particles", &particleSystem->drawMode, 1);

		//mouse interaction(blowing) enable
		
		//InputFloat("Gravity", &gravity, 0.0f, 0.0f, "%2.2f m/s^2"); SameLine();
		//SliderFloat("Gravity", &gravity, -10.0f, 10.0f, "%2.2f m/s^2");
		

		Checkbox("Performance Mode", &particleSystem->performanceMode); SameLine();
		HelpMarker("Lock the FPS to 30 for a smoother experience on lowered end hardward.");
		Checkbox("Pause", &pause);

		//change geometry and bounds damping

		//very risky, but allow the user to change DT for slow motion, maybe just have toggles for 1.0x 0.5x 0.25x?

		Unindent();
	}

	//visual settings
	if (CollapsingHeader("Visual Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ColorEdit4("Background", (float*)&backgroundColor, NULL)) {
			glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
		}
		if (ColorEdit4("Geometry", (float*)&geometryColor, NULL)) {
			particleSystem->updateLineColor();
		}
	}

	End();
}

void GUI::render() {
	Render();
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
}