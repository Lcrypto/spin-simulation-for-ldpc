// #include <QtWidgets>

#include "MainWindow.h"
#include "SettingsWidget.h"
#include "SpinWidget.h"

#include "Interface_Configurations.h"
#include "Interface_Transitions.h"
#include "Interface_Log.h"

#include <iostream>
#include <memory>

// TODO: Replace these
#include "Vectormath.h"
#include "Exception.h"
#include "Interface_State.h"
/////

SettingsWidget::SettingsWidget(std::shared_ptr<State> state, SpinWidget *spinWidget)
{
	this->state = state;
    _spinWidget = spinWidget;

	// Setup User Interface
	this->setupUi(this);

	// We use a regular expression (regex) to filter the input into the lineEdits
	QRegularExpression re("[+|-]?[\\d]*[\\.]?[\\d]*");
	this->number_validator = new QRegularExpressionValidator(re);
	QRegularExpression re2("[\\d]*[\\.]?[\\d]*");
	this->number_validator_unsigned = new QRegularExpressionValidator(re2);
	// Setup the validators for the various input fields
	this->Setup_Input_Validators();

	// Setup Configurations Tab
	//this->greater = true;
	//this->pushButton_GreaterLesser->setText("Greater");

	// Setup Transitions Tab
	this->lineEdit_Transition_Homogeneous_Last->setText(QString::number(Chain_Get_NOI(this->state.get())));

	// Setup Interactions Tab
	if (this->state->active_image->is_isotropic) this->tabWidget_Settings->removeTab(3);
	else this->tabWidget_Settings->removeTab(2);

	// Load information from Spin Systems
	this->update();

	// Connect slots
	this->Setup_Configurations_Slots();
	this->Setup_Transitions_Slots();
	this->Setup_Hamiltonian_Isotropic_Slots();
	this->Setup_Hamiltonian_Anisotropic_Slots();
	this->Setup_Parameters_Slots();
	this->Setup_Visualization_Slots();
}

void SettingsWidget::update()
{
	// Load Hamiltonian Contents
	if (this->state->active_image->is_isotropic) this->Load_Hamiltonian_Isotropic_Contents();
	else this->Load_Hamiltonian_Anisotropic_Contents();
	// Load Parameters Contents
	this->Load_Parameters_Contents();
	// ToDo: Also update Debug etc!
	this->Load_Visualization_Contents();
}


// -----------------------------------------------------------------------------------
// --------------------- Configurations and Transitions ------------------------------
// -----------------------------------------------------------------------------------

void SettingsWidget::randomPressed()
{
	Log_Send(state.get(), DEBUG, Log_Sender::UI, "button Random");
	Configuration_Random(this->state.get());
	this->configurationAddNoise();
	print_Energies_to_console();
}
void SettingsWidget::minusZ()
{
	Configuration_MinusZ(this->state.get());
	this->configurationAddNoise();
	print_Energies_to_console();
}
void SettingsWidget::plusZ()
{
	Configuration_PlusZ(this->state.get());
	this->configurationAddNoise();
	print_Energies_to_console();
}

void SettingsWidget::create_Skyrmion()
{
	Log_Send(state.get(), DEBUG, UI, "button createSkyrmion");
	double speed = lineEdit_sky_order->text().toDouble();
	double phase = lineEdit_sky_phase->text().toDouble();
	bool upDown = checkBox_sky_UpDown->isChecked();
	bool achiral = checkBox_sky_Achiral->isChecked();
	bool rl = checkBox_sky_RL->isChecked();
	bool experimental = checkBox_sky_experimental->isChecked();
	std::vector<double> pos =
	{
		lineEdit_sky_posx->text().toDouble() + state->active_image->geometry->center[0],
		lineEdit_sky_posy->text().toDouble() + state->active_image->geometry->center[1],
		lineEdit_sky_posz->text().toDouble() + state->active_image->geometry->center[2]
	};
	double rad = lineEdit_sky_rad->text().toDouble();
	Configuration_Skyrmion(this->state.get(), pos.data(), rad, speed, phase, upDown, achiral, rl, experimental);
	this->configurationAddNoise();
	print_Energies_to_console();
}

void SettingsWidget::create_SpinSpiral()
{
	Log_Send(state.get(), DEBUG, UI, "button createSpinSpiral");
	double direction[3] = { lineEdit_SS_dir_x->text().toDouble(), lineEdit_SS_dir_y->text().toDouble(), lineEdit_SS_dir_z->text().toDouble() };
	double axis[3] = { lineEdit_SS_axis_x->text().toDouble(), lineEdit_SS_axis_y->text().toDouble(), lineEdit_SS_axis_z->text().toDouble() };
	double period = lineEdit_SS_period->text().toDouble();
	const char * direction_type;
	// And now an ugly workaround because the QT people are too stupid to fix a Bug with QString::toStdString on Windows...
	if (comboBox_SS->currentText() == "Real Lattice") direction_type = "Real Lattice";
	else if (comboBox_SS->currentText() == "Reciprocal Lattice") direction_type = "Reciprocal Lattice";
	else if (comboBox_SS->currentText() == "Real Space") direction_type = "Real Space";
	Configuration_SpinSpiral(this->state.get(), direction_type, direction, axis, period);
	this->configurationAddNoise();
	print_Energies_to_console();
}

void SettingsWidget::domainWallPressed()
{
	Log_Send(state.get(), DEBUG, UI, "button DomainWall");
	double vec[3] = { lineEdit_vx->text().toDouble(), lineEdit_vy->text().toDouble(), lineEdit_vz->text().toDouble() };
	double pos[3] = { lineEdit_posx->text().toDouble(), lineEdit_posy->text().toDouble(), lineEdit_posz->text().toDouble() };
	Configuration_DomainWall(this->state.get(), pos, vec, this->radioButton_DW_greater->isChecked());
	this->configurationAddNoise();
	print_Energies_to_console();
}

void SettingsWidget::configurationAddNoise()
{
	// Add Noise
	if (this->checkBox_Configuration_Noise->isChecked())
	{
		double temperature = lineEdit_Configuration_Noise->text().toDouble();
		Configuration_Add_Noise_Temperature(this->state.get(), temperature);
	}
}

void SettingsWidget::homogeneousTransitionPressed()
{
	int idx_1 = this->lineEdit_Transition_Homogeneous_First->text().toInt() - 1;
	int idx_2 = this->lineEdit_Transition_Homogeneous_Last->text().toInt() - 1;

	// Check the validity of the indices
	if (idx_1 < 0 || idx_1 >= Chain_Get_NOI(this->state.get()))
	{
		Log_Send(state.get(), ERROR, UI, "First index for homogeneous transition is invalid! setting to 1...");
		this->lineEdit_Transition_Homogeneous_First->setText(QString::number(1));
	}
	if (idx_1 < 0 || idx_1 >= Chain_Get_NOI(this->state.get()))
	{
		Log_Send(state.get(), ERROR, UI, "First index for homogeneous transition is invalid! setting to 1...");
		this->lineEdit_Transition_Homogeneous_First->setText(QString::number(1));
	}
	if (idx_1 == idx_2)
	{
		Log_Send(state.get(), ERROR, UI, "Indices are equal in homogeneous transition! Aborting...");
		return;
	}
	if (idx_2 < idx_1)
	{
		Log_Send(state.get(), ERROR, UI, "Index 2 is smaller than index 1 in homogeneous transition! Aborting...");
		return;
	}

	// Do the transition
	Transition_Homogeneous(this->state.get(), idx_1, idx_2);

	// Add Noise
	if (this->checkBox_Transition_Noise->isChecked())
	{
		double temperature = lineEdit_Transition_Noise->text().toDouble();
		Transition_Add_Noise_Temperature(this->state.get(), temperature, idx_1, idx_2);
	}
}


// -----------------------------------------------------------------------------------
// --------------------- Load Contents -----------------------------------------------
// -----------------------------------------------------------------------------------


void SettingsWidget::Load_Parameters_Contents()
{
	// LLG Damping
	this->lineEdit_Damping->setText(QString::number(state->active_image->llg_parameters->damping));
	// Converto to PicoSeconds
	this->lineEdit_dt->setText(QString::number(state->active_image->llg_parameters->dt /std::pow(10, -12) * Utility::Vectormath::MuB()/1.760859644/std::pow(10, 11)));
	// LLG Iteration Params
	this->lineEdit_llg_n_iterations->setText(QString::number(state->active_image->llg_parameters->n_iterations));
	this->lineEdit_llg_log_steps->setText(QString::number(state->active_image->llg_parameters->log_steps));
	// GNEB Interation Params
	this->lineEdit_gneb_n_iterations->setText(QString::number(state->active_chain->gneb_parameters->n_iterations));
	this->lineEdit_gneb_log_steps->setText(QString::number(state->active_chain->gneb_parameters->log_steps));

	// GNEB Spring Constant
	this->lineEdit_gneb_springconstant->setText(QString::number(this->state->active_chain->gneb_parameters->spring_constant));

	// Normal/Climbing/Falling image radioButtons
	this->radioButton_Normal->setChecked(!(this->state->active_chain->climbing_image[this->state->idx_active_image] || this->state->active_chain->falling_image[this->state->idx_active_image]));
	this->radioButton_ClimbingImage->setChecked(this->state->active_chain->climbing_image[this->state->idx_active_image]);
	this->radioButton_FallingImage->setChecked(this->state->active_chain->falling_image[this->state->idx_active_image]);
}


void SettingsWidget::Load_Hamiltonian_Isotropic_Contents()
{
	auto ham = (Engine::Hamiltonian_Isotropic*)state->active_image->hamiltonian.get();

	// Boundary conditions
	this->checkBox_iso_periodical_a->setChecked(ham->boundary_conditions[0]);
	this->checkBox_iso_periodical_b->setChecked(ham->boundary_conditions[1]);
	this->checkBox_iso_periodical_c->setChecked(ham->boundary_conditions[2]);

	// mu_s
	this->lineEdit_muSpin->setText(QString::number(ham->mu_s));

	// External magnetic field
	this->lineEdit_extH->setText(QString::number(ham->external_field_magnitude / Utility::Vectormath::MuB() / ham->mu_s));
	this->lineEdit_extHx->setText(QString::number(ham->external_field_normal[0]));
	this->lineEdit_extHy->setText(QString::number(ham->external_field_normal[1]));
	this->lineEdit_extHz->setText(QString::number(ham->external_field_normal[2]));
	if (ham->external_field_magnitude > 0.0) this->checkBox_extH->setChecked(true);
	
	// Exchange interaction
	if (ham->n_neigh_shells > 0) {
		lineEdit_exchange1->setText(QString::number(ham->jij[0]));
		lineEdit_exchange1->setEnabled(true);
	}
	else { lineEdit_exchange1->hide(); }
	if (ham->n_neigh_shells > 1) {
		lineEdit_exchange2->setText(QString::number(ham->jij[1]));
		lineEdit_exchange2->setEnabled(true);
	}
	else { lineEdit_exchange2->hide(); }
	if (ham->n_neigh_shells > 2) {
		lineEdit_exchange3->setText(QString::number(ham->jij[2]));
		lineEdit_exchange3->setEnabled(true);
	}
	else { lineEdit_exchange3->hide(); }
	if (ham->n_neigh_shells > 3) {
		lineEdit_exchange4->setText(QString::number(ham->jij[3]));
		lineEdit_exchange4->setEnabled(true);
	}
	else { lineEdit_exchange4->hide(); }
	if (ham->n_neigh_shells > 4) {
		lineEdit_exchange5->setText(QString::number(ham->jij[4]));
		lineEdit_exchange5->setEnabled(true);
	}
	else { lineEdit_exchange5->hide(); }
	checkBox_exchange->setChecked(true);

	// DMI
	this->lineEdit_dmi->setText(QString::number(ham->dij));
	if (ham->dij > 0.0) this->checkBox_dmi->setChecked(true);

	// Anisotropy
	this->lineEdit_aniso->setText(QString::number(ham->anisotropy_magnitude));
	this->lineEdit_anisox->setText(QString::number(ham->anisotropy_normal[0]));
	this->lineEdit_anisoy->setText(QString::number(ham->anisotropy_normal[1]));
	this->lineEdit_anisoz->setText(QString::number(ham->anisotropy_normal[2]));
	if (ham->anisotropy_magnitude > 0.0) this->checkBox_aniso->setChecked(true);

	// Spin polarized current (does not really belong to interactions)
	this->lineEdit_spin_torque->setText(QString::number(state->active_image->llg_parameters->stt_magnitude));
	this->lineEdit_spin_torquex->setText(QString::number(state->active_image->llg_parameters->stt_polarisation_normal[0]));
	this->lineEdit_spin_torquey->setText(QString::number(state->active_image->llg_parameters->stt_polarisation_normal[1]));
	this->lineEdit_spin_torquez->setText(QString::number(state->active_image->llg_parameters->stt_polarisation_normal[2]));
	if (state->active_image->llg_parameters->stt_magnitude > 0.0) this->checkBox_spin_torque->setChecked(true);

	// BQE
	this->lineEdit_bqe->setText(QString::number(ham->bij));
	if (ham->bij > 0.0) this->checkBox_bqe->setChecked(true);

	// FourSpin
	this->lineEdit_fourspin->setText(QString::number(ham->kijkl));
	if (ham->kijkl > 0.0) this->checkBox_fourspin->setChecked(true);

	// Temperature (does not really belong to interactions)
	this->lineEdit_temper->setText(QString::number(state->active_image->llg_parameters->temperature));
	if (state->active_image->llg_parameters->temperature > 0.0) this->checkBox_Temperature->setChecked(true);
}



void SettingsWidget::Load_Hamiltonian_Anisotropic_Contents()
{
	auto ham = (Engine::Hamiltonian_Anisotropic*)state->active_image->hamiltonian.get();

	// Boundary conditions
	this->checkBox_aniso_periodical_a->setChecked(state->active_image->hamiltonian->boundary_conditions[0]);
	this->checkBox_aniso_periodical_b->setChecked(state->active_image->hamiltonian->boundary_conditions[1]);
	this->checkBox_aniso_periodical_c->setChecked(state->active_image->hamiltonian->boundary_conditions[2]);

	// mu_s
	this->lineEdit_muSpin_aniso->setText(QString::number(ham->mu_s[0]));

	// External magnetic field
	this->lineEdit_extH_aniso->setText(QString::number(ham->external_field_magnitude[0] / Utility::Vectormath::MuB() / ham->mu_s[0]));
	this->lineEdit_extHx_aniso->setText(QString::number(ham->external_field_normal[0][0]));
	this->lineEdit_extHy_aniso->setText(QString::number(ham->external_field_normal[1][0]));
	this->lineEdit_extHz_aniso->setText(QString::number(ham->external_field_normal[2][0]));
	if (ham->external_field_magnitude[0] > 0.0) this->checkBox_extH_aniso->setChecked(true);

	// Anisotropy
	this->lineEdit_ani_aniso->setText(QString::number(ham->anisotropy_magnitude[0]));
	this->lineEdit_anix_aniso->setText(QString::number(ham->anisotropy_normal[0][0]));
	this->lineEdit_aniy_aniso->setText(QString::number(ham->anisotropy_normal[1][0]));
	this->lineEdit_aniz_aniso->setText(QString::number(ham->anisotropy_normal[2][0]));
	if (ham->anisotropy_magnitude[0] > 0.0) this->checkBox_ani_aniso->setChecked(true);

	// Spin polarised current
	this->lineEdit_stt_aniso->setText(QString::number(state->active_image->llg_parameters->stt_magnitude));
	this->lineEdit_sttx_aniso->setText(QString::number(state->active_image->llg_parameters->stt_polarisation_normal[0]));
	this->lineEdit_stty_aniso->setText(QString::number(state->active_image->llg_parameters->stt_polarisation_normal[1]));
	this->lineEdit_sttz_aniso->setText(QString::number(state->active_image->llg_parameters->stt_polarisation_normal[2]));
	if (state->active_image->llg_parameters->stt_magnitude > 0.0) this->checkBox_stt_aniso->setChecked(true);

	// Temperature
	this->lineEdit_T_aniso->setText(QString::number(state->active_image->llg_parameters->temperature));
	if (state->active_image->llg_parameters->temperature > 0.0) this->checkBox_T_aniso->setChecked(true);
}

void SettingsWidget::Load_Visualization_Contents() {
  std::string visualization_mode;
  switch (_spinWidget->visualizationMode()) {
    case GLSpins::VisualizationMode::SPHERE:
      visualization_mode = "Sphere";
      break;
    case GLSpins::VisualizationMode::SURFACE:
      visualization_mode = "Surface";
      break;
    default:
      visualization_mode = "Arrows";
      break;
  }
  for (int i = 0; i < comboBox_visualizationMode->count(); i++) {
    if (string_q2std(comboBox_visualizationMode->itemText(i)) == visualization_mode) {
      comboBox_visualizationMode->setCurrentIndex(i);
      break;
    }
  }
  
  std::string miniview_position;
  switch (_spinWidget->miniviewPosition()) {
    case GLSpins::WidgetLocation::TOP_LEFT:
      miniview_position = "Top Left";
      break;
    case GLSpins::WidgetLocation::BOTTOM_LEFT:
      miniview_position = "Bottom Left";
      break;
    case GLSpins::WidgetLocation::TOP_RIGHT:
      miniview_position = "Top Right";
      break;
    default:
      miniview_position = "Bottom Right";
      break;
  }
  for (int i = 0; i < comboBox_miniViewPosition->count(); i++) {
    if (string_q2std(comboBox_miniViewPosition->itemText(i)) == miniview_position) {
      comboBox_miniViewPosition->setCurrentIndex(i);
      break;
    }
  }
  std::string coordinatesystem_position;
  switch (_spinWidget->coordinateSystemPosition()) {
    case GLSpins::WidgetLocation::TOP_LEFT:
      coordinatesystem_position = "Top Left";
      break;
    case GLSpins::WidgetLocation::BOTTOM_LEFT:
      coordinatesystem_position = "Bottom Left";
      break;
    case GLSpins::WidgetLocation::TOP_RIGHT:
      coordinatesystem_position = "Top Right";
      break;
    default:
      coordinatesystem_position = "Bottom Right";
      break;
  }
  for (int i = 0; i < comboBox_coordinateSystemPosition->count(); i++) {
    if (string_q2std(comboBox_coordinateSystemPosition->itemText(i)) == coordinatesystem_position) {
      comboBox_coordinateSystemPosition->setCurrentIndex(i);
      break;
    }
  }
  checkBox_showMiniView->setChecked(_spinWidget->isMiniviewEnabled());
  checkBox_showCoordinateSystem->setChecked(_spinWidget->isCoordinateSystemEnabled());
  
  auto z_range = _spinWidget->zRange();
  if (z_range.x < -1) {
    z_range.x = -1;
  }
  if (z_range.x > 1) {
    z_range.x = 1;
  }
  if (z_range.y < -1) {
    z_range.y = -1;
  }
  if (z_range.y > 1) {
    z_range.y = 1;
  }
  horizontalSlider_zRangeMin->setInvertedAppearance(true);
  horizontalSlider_zRangeMin->setRange(-100, 100);
  horizontalSlider_zRangeMin->setValue((int)(-z_range.x * 100));
  horizontalSlider_zRangeMax->setRange(-100, 100);
  horizontalSlider_zRangeMax->setValue((int)(z_range.y * 100));
  horizontalSlider_zRangeMin->setTracking(true);
  horizontalSlider_zRangeMax->setTracking(true);
  
  std::string colormap = "Hue-Saturation-Value";
  switch (_spinWidget->colormap()) {
    case GLSpins::Colormap::HSV:
      break;
    case GLSpins::Colormap::RED_BLUE:
      colormap = "Z-Component: Red-Blue";
      break;
    case GLSpins::Colormap::OTHER:
      break;
    default:
      break;
  }
  for (int i = 0; i < comboBox_colormap->count(); i++) {
    if (string_q2std(comboBox_colormap->itemText(i)) == colormap) {
      comboBox_colormap->setCurrentIndex(i);
      break;
    }
  }
  
  if (_spinWidget->verticalFieldOfView() == 0) {
    radioButton_orthographicProjection->setChecked(true);
  } else {
    radioButton_perspectiveProjection->setChecked(true);
  }
  
  horizontalSlider_spherePointSize->setRange(1, 10);
  horizontalSlider_spherePointSize->setValue((int)_spinWidget->spherePointSizeRange().y);
  
  checkBox_showBoundingBox->setChecked(_spinWidget->isBoundingBoxEnabled());
  
  std::string background_color = "Black";
  if (_spinWidget->backgroundColor() == glm::vec3(1.0, 1.0, 1.0)) {
    background_color = "White";
  } else if (_spinWidget->backgroundColor() == glm::vec3(0.5, 0.5, 0.5)) {
    background_color = "Gray";
  }
  for (int i = 0; i < comboBox_backgroundColor->count(); i++) {
    if (string_q2std(comboBox_backgroundColor->itemText(i)) == background_color) {
      comboBox_backgroundColor->setCurrentIndex(i);
      break;
    }
  }
}

// -----------------------------------------------------------------------------------
// --------------------- Setters for Hamiltonians and Parameters ---------------------
// -----------------------------------------------------------------------------------


void SettingsWidget::set_parameters()
{
	// Closure to set the parameters of a specific spin system
	auto apply = [this](std::shared_ptr<Data::Spin_System> s) -> void
	{
		// Time step [ps]
		// dt = time_step [ps] * 10^-12 * gyromagnetic raio / mu_B  { / (1+damping^2)} <- not implemented
		s->llg_parameters->dt = this->lineEdit_dt->text().toDouble()*std::pow(10,-12)/Utility::Vectormath::MuB()*1.760859644*std::pow(10,11);
		// Damping
		s->llg_parameters->damping = this->lineEdit_Damping->text().toDouble();
		// n iterations
		s->llg_parameters->n_iterations = this->lineEdit_llg_n_iterations->text().toDouble();
		state->active_chain->gneb_parameters->n_iterations = this->lineEdit_gneb_n_iterations->text().toDouble();
		// log steps
		s->llg_parameters->log_steps = this->lineEdit_llg_log_steps->text().toDouble();
		state->active_chain->gneb_parameters->log_steps = this->lineEdit_gneb_log_steps->text().toDouble();\
		// Spring Constant
		state->active_chain->gneb_parameters->spring_constant = this->lineEdit_gneb_springconstant->text().toDouble();
		// Climbing/Falling Image
		state->active_chain->climbing_image[state->idx_active_image] = this->radioButton_ClimbingImage->isChecked();
		state->active_chain->falling_image[state->idx_active_image] = this->radioButton_FallingImage->isChecked();
	};

	if (this->comboBox_Parameters_ApplyTo->currentText() == "Current Image")
	{
		apply(this->state->active_image);
	}
	else if (this->comboBox_Parameters_ApplyTo->currentText() == "Current Image Chain")
	{
		for (auto sys : this->state->active_chain->images)
		{
			apply(sys);
		}
	}
	else if (this->comboBox_Parameters_ApplyTo->currentText() == "All Images")
	{
		for (auto sys : this->state->active_chain->images)
		{
			apply(sys);
		}
	}
}


void SettingsWidget::set_hamiltonian_iso()
{
	// Closure to set the parameters of a specific spin system
	auto apply = [this](std::shared_ptr<Data::Spin_System> s) -> void
	{
		auto ham = (Engine::Hamiltonian_Isotropic*)s->hamiltonian.get();

		// Boundary conditions
		s->hamiltonian->boundary_conditions[0] = this->checkBox_iso_periodical_a->isChecked();
		s->hamiltonian->boundary_conditions[1] = this->checkBox_iso_periodical_b->isChecked();
		s->hamiltonian->boundary_conditions[2] = this->checkBox_iso_periodical_c->isChecked();

		// mu_s
		ham->mu_s = lineEdit_muSpin->text().toDouble();

		// External magnetic field
		//		magnitude
		if (this->checkBox_extH->isChecked())
			ham->external_field_magnitude = this->lineEdit_extH->text().toDouble()*  ham->mu_s * Utility::Vectormath::MuB();
		else ham->external_field_magnitude = 0.0;
		//		normal
		ham->external_field_normal[0] = lineEdit_extHx->text().toDouble();
		ham->external_field_normal[1] = lineEdit_extHy->text().toDouble();
		ham->external_field_normal[2] = lineEdit_extHz->text().toDouble();
		try {
			Utility::Vectormath::Normalize(ham->external_field_normal);
		}
		catch (Utility::Exception ex) {
			if (ex == Utility::Exception::Division_by_zero) {
				ham->external_field_normal[0] = 0.0;
				ham->external_field_normal[1] = 0.0;
				ham->external_field_normal[2] = 1.0;
				Log_Send(state.get(), WARNING, UI, "B_vec = {0,0,0} replaced by {0,0,1}");
				lineEdit_extHx->setText(QString::number(0.0));
				lineEdit_extHy->setText(QString::number(0.0));
				lineEdit_extHz->setText(QString::number(1.0));
			}
			else { throw(ex); }
		}

		// Exchange
		if (checkBox_exchange->isChecked())
		{
			if (lineEdit_exchange1->isEnabled()) { ham->jij[0] = lineEdit_exchange1->text().toDouble(); }
			if (lineEdit_exchange2->isEnabled()) { ham->jij[1] = lineEdit_exchange2->text().toDouble(); }
			if (lineEdit_exchange3->isEnabled()) { ham->jij[2] = lineEdit_exchange3->text().toDouble(); }
			if (lineEdit_exchange4->isEnabled()) { ham->jij[3] = lineEdit_exchange4->text().toDouble(); }
			if (lineEdit_exchange5->isEnabled()) { ham->jij[4] = lineEdit_exchange5->text().toDouble(); }
		}
		else {
			for (int i = 0; i < ham->n_neigh_shells; ++i) {
				ham->jij[i] = 0.0;
			}
		}
		
		// DMI
		if (this->checkBox_dmi->isChecked()) ham->dij = this->lineEdit_dmi->text().toDouble();
		else ham->dij = 0.0;

		// Anisotropy
		//		magnitude
		if (this->checkBox_aniso->isChecked()) ham->anisotropy_magnitude = this->lineEdit_aniso->text().toDouble();
		else ham->anisotropy_magnitude = 0.0;
		//		normal
		ham->anisotropy_normal[0] = lineEdit_anisox->text().toDouble();
		ham->anisotropy_normal[1] = lineEdit_anisoy->text().toDouble();
		ham->anisotropy_normal[2] = lineEdit_anisoz->text().toDouble();
		try {
			Utility::Vectormath::Normalize(ham->anisotropy_normal);
		}
		catch (Utility::Exception ex) {
			if (ex == Utility::Exception::Division_by_zero) {
				ham->anisotropy_normal[0] = 0.0;
				ham->anisotropy_normal[1] = 0.0;
				ham->anisotropy_normal[2] = 1.0;
				Log_Send(state.get(), WARNING, UI, "Aniso_vec = {0,0,0} replaced by {0,0,1}");
				lineEdit_anisox->setText(QString::number(0.0));
				lineEdit_anisoy->setText(QString::number(0.0));
				lineEdit_anisoz->setText(QString::number(1.0));
			}
			else { throw(ex); }
		}

		// BQE
		if (this->checkBox_bqe->isChecked()) ham->bij = this->lineEdit_bqe->text().toDouble();
		else ham->bij = 0.0;

		// FSC
		if (this->checkBox_fourspin->isChecked()) ham->kijkl = this->lineEdit_fourspin->text().toDouble();
		else ham->kijkl = 0.0;

		// These belong in Parameters, not Hamiltonian
		// Spin polarised current
		if (this->checkBox_spin_torque->isChecked()) {
			s->llg_parameters->stt_magnitude = this->lineEdit_spin_torque->text().toDouble();
		}
		else {
			this->state->active_image->llg_parameters->stt_magnitude = 0.0;
		}
		s->llg_parameters->stt_polarisation_normal[0] = lineEdit_spin_torquex->text().toDouble();
		s->llg_parameters->stt_polarisation_normal[1] = lineEdit_spin_torquey->text().toDouble();
		s->llg_parameters->stt_polarisation_normal[2] = lineEdit_spin_torquez->text().toDouble();
		try {
			Utility::Vectormath::Normalize(state->active_image->llg_parameters->stt_polarisation_normal);
		}
		catch (Utility::Exception ex) {
			if (ex == Utility::Exception::Division_by_zero) {
				s->llg_parameters->stt_polarisation_normal[0] = 0.0;
				s->llg_parameters->stt_polarisation_normal[1] = 0.0;
				s->llg_parameters->stt_polarisation_normal[2] = 1.0;
				Log_Send(state.get(), WARNING, UI, "s_c_vec = {0,0,0} replaced by {0,0,1}");
				lineEdit_spin_torquex->setText(QString::number(0.0));
				lineEdit_spin_torquey->setText(QString::number(0.0));
				lineEdit_spin_torquez->setText(QString::number(1.0));
			}
			else { throw(ex); }
		}

		// Temperature
		if (this->checkBox_Temperature->isChecked())
			s->llg_parameters->temperature = this->lineEdit_temper->text().toDouble();
		else
			s->llg_parameters->temperature = 0.0;
	};

	if (this->comboBox_Hamiltonian_Iso_ApplyTo->currentText() == "Current Image")
	{
		apply(this->state->active_image);
	}
	else if (this->comboBox_Hamiltonian_Iso_ApplyTo->currentText() == "Current Image Chain")
	{
		for (auto sys : this->state->active_chain->images)
		{
			apply(sys);
		}
	}
	else if (this->comboBox_Hamiltonian_Iso_ApplyTo->currentText() == "All Images")
	{
		for (auto sys : this->state->active_chain->images)
		{
			apply(sys);
		}
	}
}

void SettingsWidget::set_hamiltonian_aniso()
{
	// Closure to set the parameters of a specific spin system
	auto apply = [this](std::shared_ptr<Data::Spin_System> s) -> void
	{
		std::vector<double> temp(3);
		auto ham = (Engine::Hamiltonian_Anisotropic*)s->hamiltonian.get();

		// Boundary conditions
		s->hamiltonian->boundary_conditions[0] = this->checkBox_aniso_periodical_a->isChecked();
		s->hamiltonian->boundary_conditions[1] = this->checkBox_aniso_periodical_b->isChecked();
		s->hamiltonian->boundary_conditions[2] = this->checkBox_aniso_periodical_c->isChecked();

		// mu_s
		for (auto mu_s : ham->mu_s) mu_s = this->lineEdit_muSpin_aniso->text().toDouble();

		// External magnetic field
		//		magnitude
		if (this->checkBox_extH_aniso->isChecked()) {
			for (int iatom = 0; iatom < System_Get_NOS(this->state.get()); ++iatom) {
				ham->external_field_magnitude[iatom] = this->lineEdit_extH_aniso->text().toDouble() * ham->mu_s[iatom] * Utility::Vectormath::MuB();
			}
		}
		else {
			for (int iatom = 0; iatom < System_Get_NOS(this->state.get()); ++iatom) {
				ham->external_field_magnitude[iatom] = 0.0;
			}
		}
		//		normal
		temp[0] = lineEdit_extHx_aniso->text().toDouble();
		temp[1] = lineEdit_extHy_aniso->text().toDouble();
		temp[2] = lineEdit_extHz_aniso->text().toDouble();
		try {
			Utility::Vectormath::Normalize(temp);
		}
		catch (Utility::Exception ex) {
			if (ex == Utility::Exception::Division_by_zero) {
				temp[0] = 0.0;
				temp[1] = 0.0;
				temp[2] = 1.0;
				Log_Send(state.get(), WARNING, UI, "B_vec = {0,0,0} replaced by {0,0,1}");
				lineEdit_extHx_aniso->setText(QString::number(0.0));
				lineEdit_extHy_aniso->setText(QString::number(0.0));
				lineEdit_extHz_aniso->setText(QString::number(1.0));
			}
			else { throw(ex); }
		}
		for (int iatom = 0; iatom < System_Get_NOS(this->state.get()); ++iatom) {
			ham->external_field_normal[0][iatom] = temp[0];
			ham->external_field_normal[1][iatom] = temp[1];
			ham->external_field_normal[2][iatom] = temp[2];
		}
		
		// Anisotropy
		//		magnitude
		if (this->checkBox_ani_aniso->isChecked()) {
			for (int iatom = 0; iatom < System_Get_NOS(this->state.get()); ++iatom) {
				ham->anisotropy_magnitude[iatom] = this->lineEdit_ani_aniso->text().toDouble();
			}
		}
		else {
			for (int iatom = 0; iatom < System_Get_NOS(this->state.get()); ++iatom) {
				ham->anisotropy_magnitude[iatom] = 0.0;
			}
		}
		//		normal
		temp[0] = lineEdit_anix_aniso->text().toDouble();
		temp[1] = lineEdit_aniy_aniso->text().toDouble();
		temp[2] = lineEdit_aniz_aniso->text().toDouble();
		try {
			Utility::Vectormath::Normalize(temp);
		}
		catch (Utility::Exception ex) {
			if (ex == Utility::Exception::Division_by_zero) {
				temp[0] = 0.0;
				temp[1] = 0.0;
				temp[2] = 1.0;
				Log_Send(state.get(), WARNING, UI, "ani_vec = {0,0,0} replaced by {0,0,1}");
				lineEdit_anix_aniso->setText(QString::number(0.0));
				lineEdit_aniy_aniso->setText(QString::number(0.0));
				lineEdit_aniz_aniso->setText(QString::number(1.0));
			}
			else { throw(ex); }
		}
		for (int iatom = 0; iatom < System_Get_NOS(this->state.get()); ++iatom) {
			ham->anisotropy_normal[0][iatom] = temp[0];
			ham->anisotropy_normal[1][iatom] = temp[1];
			ham->anisotropy_normal[2][iatom] = temp[2];
		}

		// TODO: Make these anisotropic for Anisotropic Hamiltonian
		//		 or move them to Parameters...
		// Spin polarised current
		if (this->checkBox_stt_aniso->isChecked())
			s->llg_parameters->stt_magnitude = this->lineEdit_stt_aniso->text().toDouble();
		else this->state->active_image->llg_parameters->stt_magnitude = 0.0;
		s->llg_parameters->stt_polarisation_normal[0] = lineEdit_sttx_aniso->text().toDouble();
		s->llg_parameters->stt_polarisation_normal[1] = lineEdit_stty_aniso->text().toDouble();
		s->llg_parameters->stt_polarisation_normal[2] = lineEdit_sttz_aniso->text().toDouble();
		try {
			Utility::Vectormath::Normalize(state->active_image->llg_parameters->stt_polarisation_normal);
		}
		catch (Utility::Exception ex) {
			if (ex == Utility::Exception::Division_by_zero) {
				s->llg_parameters->stt_polarisation_normal[0] = 0.0;
				s->llg_parameters->stt_polarisation_normal[1] = 0.0;
				s->llg_parameters->stt_polarisation_normal[2] = 1.0;
				Log_Send(state.get(), WARNING, UI, "s_c_vec = {0,0,0} replaced by {0,0,1}");
				lineEdit_sttx_aniso->setText(QString::number(0.0));
				lineEdit_stty_aniso->setText(QString::number(0.0));
				lineEdit_sttz_aniso->setText(QString::number(1.0));
			}
			else { throw(ex); }
		}

		// Temperature
		if (this->checkBox_T_aniso->isChecked())
			s->llg_parameters->temperature = this->lineEdit_T_aniso->text().toDouble();
		else
			s->llg_parameters->temperature = 0.0;
	};

	if (this->comboBox_Hamiltonian_Ani_ApplyTo->currentText() == "Current Image")
	{
		apply(this->state->active_image);
	}
	else if (this->comboBox_Hamiltonian_Ani_ApplyTo->currentText() == "Current Image Chain")
	{
		for (auto sys : this->state->active_chain->images)
		{
			apply(sys);
		}
	}
	else if (this->comboBox_Hamiltonian_Ani_ApplyTo->currentText() == "All Images")
	{
		for (auto sys : this->state->active_chain->images)
		{
			apply(sys);
		}
	}
}

void SettingsWidget::set_visualization()
{
  GLSpins::VisualizationMode visualization_mode = GLSpins::VisualizationMode::ARROWS;
  if (comboBox_visualizationMode->currentText() == "Surface") {
    visualization_mode = GLSpins::VisualizationMode::SURFACE;
  } else if (comboBox_visualizationMode->currentText() == "Sphere") {
    visualization_mode = GLSpins::VisualizationMode::SPHERE;
  }
  _spinWidget->setVisualizationMode(visualization_mode);
  
  _spinWidget->enableMiniview(checkBox_showMiniView->isChecked());
  _spinWidget->enableCoordinateSystem(checkBox_showCoordinateSystem->isChecked());
  GLSpins::WidgetLocation miniview_position = GLSpins::WidgetLocation::BOTTOM_RIGHT;
  if (comboBox_miniViewPosition->currentText() == "Top Left") {
    miniview_position = GLSpins::WidgetLocation::TOP_LEFT;
  } else if (comboBox_miniViewPosition->currentText() == "Bottom Left") {
    miniview_position = GLSpins::WidgetLocation::BOTTOM_LEFT;
  } else if (comboBox_miniViewPosition->currentText() == "Top Right") {
    miniview_position = GLSpins::WidgetLocation::TOP_RIGHT;
  }
  GLSpins::WidgetLocation coordinatesystem_position = GLSpins::WidgetLocation::BOTTOM_RIGHT;
  if (comboBox_coordinateSystemPosition->currentText() == "Top Left") {
    coordinatesystem_position = GLSpins::WidgetLocation::TOP_LEFT;
  } else if (comboBox_coordinateSystemPosition->currentText() == "Bottom Left") {
    coordinatesystem_position = GLSpins::WidgetLocation::BOTTOM_LEFT;
  } else if (comboBox_coordinateSystemPosition->currentText() == "Top Right") {
    coordinatesystem_position = GLSpins::WidgetLocation::TOP_RIGHT;
  }
  _spinWidget->setMiniviewPosition(miniview_position);
  _spinWidget->setCoordinateSystemPosition(coordinatesystem_position);
  
  
  float z_range_min = -horizontalSlider_zRangeMin->value()/100.0;
  float z_range_max = horizontalSlider_zRangeMax->value()/100.0;
  if (z_range_min > z_range_max) {
    float t = z_range_min;
    z_range_min = z_range_max;
    z_range_max = t;
  }
  horizontalSlider_zRangeMin->blockSignals(true);
  horizontalSlider_zRangeMax->blockSignals(true);
  horizontalSlider_zRangeMin->setValue((int)(-z_range_min * 100));
  horizontalSlider_zRangeMax->setValue((int)(z_range_max * 100));
  horizontalSlider_zRangeMin->blockSignals(false);
  horizontalSlider_zRangeMax->blockSignals(false);
  
  glm::vec2 z_range(z_range_min, z_range_max);
  _spinWidget->setZRange(z_range);
  
  GLSpins::Colormap colormap = GLSpins::Colormap::HSV;
  if (comboBox_colormap->currentText() == "Z-Component: Red-Blue") {
    colormap = GLSpins::Colormap::RED_BLUE;
  }
  _spinWidget->setColormap(colormap);
  
  if (radioButton_orthographicProjection->isChecked()) {
    _spinWidget->setVerticalFieldOfView(0.0);
  } else {
    _spinWidget->setVerticalFieldOfView(45.0);
  }
  
  _spinWidget->enableBoundingBox(checkBox_showBoundingBox->isChecked());
  
  _spinWidget->setSpherePointSizeRange(glm::vec2(1.0f, 1.0f*horizontalSlider_spherePointSize->value()));
  
  glm::vec3 background_color(0.0, 0.0, 0.0);
  glm::vec3 bounding_box_color(1.0, 1.0, 1.0);
  if (comboBox_backgroundColor->currentText() == "White") {
    background_color = glm::vec3(1.0, 1.0, 1.0);
    bounding_box_color = glm::vec3(0.0, 0.0, 0.0);
  } else if (comboBox_backgroundColor->currentText() == "Gray") {
    background_color = glm::vec3(0.5, 0.5, 0.5);
    bounding_box_color = glm::vec3(1.0, 1.0, 1.0);
  }
  _spinWidget->setBackgroundColor(background_color);
  _spinWidget->setBoundingBoxColor(bounding_box_color);
}


// -----------------------------------------------------------------------------------
// --------------------- Utilities ---------------------------------------------------
// -----------------------------------------------------------------------------------


void SettingsWidget::SelectTab(int index)
{
	this->tabWidget_Settings->setCurrentIndex(index);
}


void SettingsWidget::print_Energies_to_console()
{
	state->active_image->UpdateEnergy();
	std::cout << "E_tot = " << state->active_image->E / System_Get_NOS(this->state.get()) << "  ||| Zeeman = ";
	std::cout << state->active_image->E_array[ENERGY_POS_ZEEMAN] / System_Get_NOS(this->state.get()) << "  | Aniso = "
		<< state->active_image->E_array[ENERGY_POS_ANISOTROPY] / System_Get_NOS(this->state.get()) << "  | Exchange = "
		<< state->active_image->E_array[ENERGY_POS_EXCHANGE] / System_Get_NOS(this->state.get()) << "  | DMI = "
		<< state->active_image->E_array[ENERGY_POS_DMI] / System_Get_NOS(this->state.get()) << "  | BQC = "
		<< state->active_image->E_array[ENERGY_POS_BQC] / System_Get_NOS(this->state.get()) << "  | FourSC = "
		<< state->active_image->E_array[ENERGY_POS_FSC] / System_Get_NOS(this->state.get()) << "  | DD = "
		<< state->active_image->E_array[ENERGY_POS_DD] / System_Get_NOS(this->state.get()) << std::endl;
}


// -----------------------------------------------------------------------------------
// --------------------- Setup functions for Slots and Validators --------------------
// -----------------------------------------------------------------------------------

void SettingsWidget::Setup_Hamiltonian_Isotropic_Slots()
{
	// Boundary conditions
	connect(this->checkBox_iso_periodical_a, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->checkBox_iso_periodical_b, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->checkBox_iso_periodical_c, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	// mu_s
	connect(this->lineEdit_muSpin, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// External Magnetic Field
	connect(this->checkBox_extH, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_extH, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_extHx, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_extHy, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_extHz, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// Exchange
	connect(this->lineEdit_exchange1, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_exchange2, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_exchange3, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_exchange4, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_exchange5, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->checkBox_exchange, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	// DMI
	connect(this->checkBox_dmi, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_dmi, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// Anisotropy
	connect(this->checkBox_aniso, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_anisox, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_anisoy, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_anisoz, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// Biquadratic Exchange
	connect(this->checkBox_bqe, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_bqe, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// FourSpin Interaction
	connect(this->checkBox_fourspin, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_fourspin, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// Spin Torque (does not really belong to interactions)
	connect(this->checkBox_spin_torque, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_spin_torque, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_spin_torquex, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_spin_torquey, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_spin_torquez, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));
	// Temperature (does not really belong to interactions)
	connect(this->checkBox_Temperature, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_iso()));
	connect(this->lineEdit_temper, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_iso()));

}

void SettingsWidget::Setup_Hamiltonian_Anisotropic_Slots()
{
	// Boundary Conditions
	connect(this->checkBox_aniso_periodical_a, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	connect(this->checkBox_aniso_periodical_b, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	connect(this->checkBox_aniso_periodical_c, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	// mu_s
	connect(this->lineEdit_muSpin_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	// External Field
	connect(this->checkBox_extH_aniso, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_extH_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_extHx_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_extHy_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_extHz_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	// Anisotropy
	connect(this->checkBox_ani_aniso, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_ani_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_anix_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_aniy_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_aniz_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	// Spin polarised current
	connect(this->checkBox_stt_aniso, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_stt_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_sttx_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_stty_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_sttz_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
	// Temperature
	connect(this->checkBox_T_aniso, SIGNAL(stateChanged(int)), this, SLOT(set_hamiltonian_aniso()));
	connect(this->lineEdit_T_aniso, SIGNAL(returnPressed()), this, SLOT(set_hamiltonian_aniso()));
}

void SettingsWidget::Setup_Parameters_Slots()
{
	// LLG Damping
	connect(this->lineEdit_Damping, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	connect(this->lineEdit_dt, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	// LLG iteration params
	connect(this->lineEdit_llg_n_iterations, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	connect(this->lineEdit_llg_log_steps, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	// GNEB iteration params
	connect(this->lineEdit_llg_n_iterations, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	connect(this->lineEdit_llg_log_steps, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	// GNEB Spring Constant
	connect(this->lineEdit_gneb_springconstant, SIGNAL(returnPressed()), this, SLOT(set_parameters()));
	// Normal/Climbing/Falling image radioButtons
	connect(this->radioButton_Normal, SIGNAL(clicked()), this, SLOT(set_parameters()));
	connect(this->radioButton_ClimbingImage, SIGNAL(clicked()), this, SLOT(set_parameters()));
	connect(this->radioButton_FallingImage, SIGNAL(clicked()), this, SLOT(set_parameters()));
}

void SettingsWidget::Setup_Configurations_Slots()
{
	// Random
	connect(this->pushButton_Random, SIGNAL(clicked()), this, SLOT(randomPressed()));
	// Domain Wall
	connect(this->pushButton_DomainWall, SIGNAL(clicked()), this, SLOT(domainWallPressed()));
	// Homogeneous
	connect(this->pushButton_plusZ, SIGNAL(clicked()), this, SLOT(plusZ()));
	connect(this->pushButton_minusZ, SIGNAL(clicked()), this, SLOT(minusZ()));
	// Skyrmion
	connect(this->pushButton_skyrmion, SIGNAL(clicked()), this, SLOT(create_Skyrmion()));
	// Spin Spiral
	connect(this->pushButton_SS, SIGNAL(clicked()), this, SLOT(create_SpinSpiral()));

	// Domain Wall LineEdits
	connect(this->lineEdit_vx, SIGNAL(returnPressed()), this, SLOT(domainWallPressed()));
	connect(this->lineEdit_vy, SIGNAL(returnPressed()), this, SLOT(domainWallPressed()));
	connect(this->lineEdit_vz, SIGNAL(returnPressed()), this, SLOT(domainWallPressed()));
	connect(this->lineEdit_posx, SIGNAL(returnPressed()), this, SLOT(domainWallPressed()));
	connect(this->lineEdit_posy, SIGNAL(returnPressed()), this, SLOT(domainWallPressed()));
	connect(this->lineEdit_posz, SIGNAL(returnPressed()), this, SLOT(domainWallPressed()));

	// Skyrmion LineEdits
	connect(this->lineEdit_sky_order, SIGNAL(returnPressed()), this, SLOT(create_Skyrmion()));
	connect(this->lineEdit_sky_phase, SIGNAL(returnPressed()), this, SLOT(create_Skyrmion()));
	connect(this->lineEdit_sky_posx, SIGNAL(returnPressed()), this, SLOT(create_Skyrmion()));
	connect(this->lineEdit_sky_posy, SIGNAL(returnPressed()), this, SLOT(create_Skyrmion()));
	connect(this->lineEdit_sky_posz, SIGNAL(returnPressed()), this, SLOT(create_Skyrmion()));
	connect(this->lineEdit_sky_rad, SIGNAL(returnPressed()), this, SLOT(create_Skyrmion()));

	// SpinSpiral LineEdits
	connect(this->lineEdit_SS_dir_x, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));
	connect(this->lineEdit_SS_dir_y, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));
	connect(this->lineEdit_SS_dir_z, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));
	connect(this->lineEdit_SS_axis_x, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));
	connect(this->lineEdit_SS_axis_y, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));
	connect(this->lineEdit_SS_axis_z, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));
	connect(this->lineEdit_SS_period, SIGNAL(returnPressed()), this, SLOT(create_SpinSpiral()));

}

void SettingsWidget::Setup_Transitions_Slots()
{
	// Homogeneous Transition
	connect(this->pushButton_Transition_Homogeneous, SIGNAL(clicked()), this, SLOT(homogeneousTransitionPressed()));
}

void SettingsWidget::Setup_Visualization_Slots()
{
  connect(comboBox_visualizationMode, SIGNAL(currentIndexChanged(int)), this, SLOT(set_visualization()));
  connect(checkBox_showMiniView, SIGNAL(stateChanged(int)), this, SLOT(set_visualization()));
  connect(checkBox_showCoordinateSystem, SIGNAL(stateChanged(int)), this, SLOT(set_visualization()));
  connect(checkBox_showBoundingBox, SIGNAL(stateChanged(int)), this, SLOT(set_visualization()));
  connect(comboBox_miniViewPosition, SIGNAL(currentIndexChanged(int)), this, SLOT(set_visualization()));
  connect(comboBox_coordinateSystemPosition, SIGNAL(currentIndexChanged(int)), this, SLOT(set_visualization()));
  connect(horizontalSlider_zRangeMin, SIGNAL(valueChanged(int)), this, SLOT(set_visualization()));
  connect(horizontalSlider_zRangeMax, SIGNAL(valueChanged(int)), this, SLOT(set_visualization()));
  connect(comboBox_colormap, SIGNAL(currentIndexChanged(int)), this, SLOT(set_visualization()));
  connect(radioButton_perspectiveProjection, SIGNAL(toggled(bool)), this, SLOT(set_visualization()));
  connect(radioButton_orthographicProjection, SIGNAL(toggled(bool)), this, SLOT(set_visualization()));
  connect(comboBox_backgroundColor, SIGNAL(currentIndexChanged(int)), this, SLOT(set_visualization()));
  connect(horizontalSlider_spherePointSize, SIGNAL(valueChanged(int)), this, SLOT(set_visualization()));
}

void SettingsWidget::Setup_Input_Validators()
{
	// Isotropic Hamiltonian
	//		mu_s
	this->lineEdit_muSpin->setValidator(this->number_validator);
	//		external field
	this->lineEdit_extH->setValidator(this->number_validator);
	this->lineEdit_extHx->setValidator(this->number_validator);
	this->lineEdit_extHy->setValidator(this->number_validator);
	this->lineEdit_extHz->setValidator(this->number_validator);
	//		exchange
	this->lineEdit_exchange1->setValidator(this->number_validator);
	this->lineEdit_exchange2->setValidator(this->number_validator);
	this->lineEdit_exchange3->setValidator(this->number_validator);
	this->lineEdit_exchange4->setValidator(this->number_validator);
	this->lineEdit_exchange5->setValidator(this->number_validator);
	//		DMI
	this->lineEdit_dmi->setValidator(this->number_validator);
	//		anisotropy
	this->lineEdit_aniso->setValidator(this->number_validator);
	this->lineEdit_anisox->setValidator(this->number_validator);
	this->lineEdit_anisoy->setValidator(this->number_validator);
	this->lineEdit_anisoz->setValidator(this->number_validator);
	//		spin polarised current
	this->lineEdit_spin_torque->setValidator(this->number_validator);
	this->lineEdit_spin_torquex->setValidator(this->number_validator);
	this->lineEdit_spin_torquey->setValidator(this->number_validator);
	this->lineEdit_spin_torquez->setValidator(this->number_validator);
	//		BQE
	this->lineEdit_bqe->setValidator(this->number_validator);
	//		FSC
	this->lineEdit_fourspin->setValidator(this->number_validator);
	//		temperature
	this->lineEdit_temper->setValidator(this->number_validator_unsigned);

	// Anisotropic Hamiltonian
	//		mu_s
	this->lineEdit_muSpin_aniso->setValidator(this->number_validator);
	//		external field
	this->lineEdit_extH_aniso->setValidator(this->number_validator);
	this->lineEdit_extHx_aniso->setValidator(this->number_validator);
	this->lineEdit_extHy_aniso->setValidator(this->number_validator);
	this->lineEdit_extHz_aniso->setValidator(this->number_validator);
	//		anisotropy
	this->lineEdit_ani_aniso->setValidator(this->number_validator);
	this->lineEdit_anix_aniso->setValidator(this->number_validator);
	this->lineEdit_aniy_aniso->setValidator(this->number_validator);
	this->lineEdit_aniz_aniso->setValidator(this->number_validator);
	//		spin polarised current
	this->lineEdit_stt_aniso->setValidator(this->number_validator);
	this->lineEdit_sttx_aniso->setValidator(this->number_validator);
	this->lineEdit_stty_aniso->setValidator(this->number_validator);
	this->lineEdit_sttz_aniso->setValidator(this->number_validator);
	//		temperature
	this->lineEdit_T_aniso->setValidator(this->number_validator_unsigned);

	// Configurations
	this->lineEdit_Configuration_Noise->setValidator(this->number_validator_unsigned);
	//		Skyrmion
	this->lineEdit_sky_order->setValidator(this->number_validator);
	this->lineEdit_sky_phase->setValidator(this->number_validator);
	this->lineEdit_sky_rad->setValidator(this->number_validator);
	this->lineEdit_sky_posx->setValidator(this->number_validator);
	this->lineEdit_sky_posy->setValidator(this->number_validator);
	this->lineEdit_sky_posz->setValidator(this->number_validator);
	//		Spin Spiral
	this->lineEdit_SS_dir_x->setValidator(this->number_validator);
	this->lineEdit_SS_dir_y->setValidator(this->number_validator);
	this->lineEdit_SS_dir_z->setValidator(this->number_validator);
	this->lineEdit_SS_axis_x->setValidator(this->number_validator);
	this->lineEdit_SS_axis_y->setValidator(this->number_validator);
	this->lineEdit_SS_axis_z->setValidator(this->number_validator);
	this->lineEdit_SS_period->setValidator(this->number_validator);
	//		Domain Wall
	this->lineEdit_vx->setValidator(this->number_validator);
	this->lineEdit_vy->setValidator(this->number_validator);
	this->lineEdit_vz->setValidator(this->number_validator);
	this->lineEdit_posx->setValidator(this->number_validator);
	this->lineEdit_posy->setValidator(this->number_validator);
	this->lineEdit_posz->setValidator(this->number_validator);

	// Transitions
	this->lineEdit_Transition_Noise->setValidator(this->number_validator_unsigned);
	this->lineEdit_Transition_Homogeneous_First->setValidator(this->number_validator_unsigned);
	this->lineEdit_Transition_Homogeneous_Last->setValidator(this->number_validator_unsigned);

	// Parameters
	//		LLG
	this->lineEdit_Damping->setValidator(this->number_validator);
	this->lineEdit_dt->setValidator(this->number_validator);
	//		GNEB
	this->lineEdit_gneb_springconstant->setValidator(this->number_validator);
}