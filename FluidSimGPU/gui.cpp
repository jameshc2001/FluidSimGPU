#include "gui.h"

using namespace ImGui;
using namespace guiVariables;

//taken from imgui_demo, copy past is allowed
// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void GUI::setScaling(float scaleFactor) {
	//reset to default scaling
	if (prevScaleFactor != 0) {
		ImGui::GetStyle().ScaleAllSizes(1.0f / scaleFactor);
	}

	ImGui::GetIO().Fonts->Clear();
	ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/font/OpenSans-Regular.ttf", (float)(constants::FONT_SIZE * scaleFactor));
	ImGui_ImplOpenGL3_CreateFontsTexture();
	ImGui::GetStyle().ScaleAllSizes(scaleFactor);
	prevScaleFactor = scaleFactor;
}

void GUI::initialise(GLFWwindow* _window, ParticleSystem* _particleSystem) {
	window = _window;
	particleSystem = _particleSystem;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	GetStyle().FrameRounding = 4.0f;
	GetStyle().IndentSpacing = 6.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

void GUI::update() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	bool showDemoWindow = true;
	ImGui::ShowDemoWindow(&showDemoWindow);

	ImGui::Begin("Tool Box", NULL, ImGuiWindowFlags_NoCollapse);

	//add framerate!

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
			Text("Fluid Creation Settings");
		}
		else {
			//show options for geometry (select curved or line mode)
			Text("Geometry Creation Settings");
		}
		Unindent();
	}

	//scene controls (delete all particles, geometry, save, load)
	if (CollapsingHeader("Scene Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();
		//if (Button("Reset Particles")) sim->removeAllParticles();
		//if (Button("Reset Geometry")) sim->removeAllGeometry();
		//if (Button("Save State")) sim->saveState();
		//if (Button("Load State")) sim->loadState();
		Unindent();
	}

	//simulation settings
	if (CollapsingHeader("Simulation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		Indent();

		//put a reset button here
		if (Button("Reset")) {
			gravity = constants::GRAVITY;
		}

		//mouse interaction(blowing) enable
		
		//InputFloat("Gravity", &gravity, 0.0f, 0.0f, "%2.2f m/s^2"); SameLine();
		//SliderFloat("Gravity", &gravity, -10.0f, 10.0f, "%2.2f m/s^2");
		DragFloat("Gravity", &gravity, 0.05f, -10.0f, 10.0f, "%2.2f m/s^2", ImGuiSliderFlags_AlwaysClamp); SameLine();
		HelpMarker("You can click and drag the value,\nor double click to type in your own.");

		//change geometry and bounds damping

		//very risky, but allow the user to change DT for slow motion, maybe just have toggles for 1.0x 0.5x 0.25x?

		Unindent();
	}

	//particle types

	//visual settings (colors of geometry and background)

	//simulation metrics

	//SameLine(); HelpMarker("Choose what will be created (and destroyed) when you click inside the simulation");
	ImGui::End();
}

void GUI::render() {
	ImGui::Render();
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}