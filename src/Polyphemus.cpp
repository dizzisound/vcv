#include "TechTechTechnologies.hpp"
#define N 3
#define MIN(A,B) ((A<B)? A : B)
#define MAX(A,B) ((A>B)? A : B)
#define CLIP(A, B, C) MIN(MAX(A,B),C)

typedef struct
{
    float data[2] = {0};
    float a;
    float b;
    int head = 0;
} biquad;



struct Polyphemus : Module {
	enum ParamIds {
        GAIN_PARAM,
        RADIUS_PARAM = GAIN_PARAM+N,
        ANGLE_PARAM = RADIUS_PARAM+N,
        RADCV_PARAM = ANGLE_PARAM+N,
        ANGCV_PARAM = RADCV_PARAM+N,
		NUM_PARAMS = ANGCV_PARAM+N
	};
	enum InputIds {
		SIGNAL_INPUT,
        RADIUS_INPUT = SIGNAL_INPUT+N,
        ANGLE_INPUT = RADIUS_INPUT+N,
		NUM_INPUTS = ANGLE_INPUT+N
	};
	enum OutputIds {
        SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    int ready = 0;

    Label* testLabel;

    biquad filters[N];

	Polyphemus() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

/* IIR coefficients
A = 2*r*cos(theta)
B = r^2
*/

void Polyphemus::step() {
	float deltaTime = 1.0 / engineGetSampleRate();

    if(ready == 0) return;

    float x, y;
    float r, a;
    float gain;

    gain = params[GAIN_PARAM].value;

    x = inputs[SIGNAL_INPUT].value*gain;

    for(int j = 0; j < N; ++j)
    { 

        //retrieve pole params from inputs
        //radius is -1 ~ 1, angle is 0 ~ 3.14
        //inputs are 0 ~ 10 w/ attenuverters
        r = params[RADIUS_PARAM+j].value
          + params[RADCV_PARAM+j].value*inputs[RADIUS_INPUT+j].value/10;
        a = params[ANGLE_PARAM+j].value
          + params[ANGCV_PARAM+j].value*inputs[ANGLE_INPUT+j].value*3.14/10;

        //clip to +/- 1
        r = CLIP(-1, r, 1);
        a = CLIP(0, a, 6.28);

/*
        if(j == 1)
        {
            char tstr[256];
            sprintf(tstr, "%f, %f", r, a);
            if(testLabel)
                testLabel->text = tstr;
        }
*/

        //Set filter params from inputs

        filters[j].a = -2*r*cos(a);
        filters[j].b = r*r;

        //apply filter to value

        y = x;
        y -= filters[j].a*filters[j].data[filters[j].head];
        filters[j].head ^= 1;
        y -= filters[j].b*filters[j].data[filters[j].head];

        filters[j].data[filters[j].head] = y;
        x = y;

    }

    outputs[SIGNAL_OUTPUT].value = x;
    //set output to value*gain

}


PolyphemusWidget::PolyphemusWidget() {
	Polyphemus *module = new Polyphemus();
	setModule(module);
	box.size = Vec(9* RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Polyphemus.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createScrew<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


    float xoff, yoff;

    xoff = 17.5;
    yoff = 380-302.5-25;

    addInput(createInput<PJ301MPort>(
        Vec(xoff, yoff), module, Polyphemus::SIGNAL_INPUT
        ));


    addParam(createParam<RoundBlackKnob>(
        Vec(xoff+28, yoff-6.5), module, Polyphemus::GAIN_PARAM,
        0, 2, 1
        ));



    xoff = 89;

    addOutput(createOutput<PJ301MPort>(
        Vec(xoff, yoff), module, Polyphemus::SIGNAL_OUTPUT
        ));



    for(int j = 0; j < N; ++j)
    {
        xoff = 17.5;
        yoff = 380-232-25+j*85;

        addInput(createInput<PJ301MPort>(
            Vec(xoff, yoff), module, Polyphemus::RADIUS_INPUT+j
            ));

        addParam(createParam<RoundTinyBlackKnob>(
            Vec(xoff+34, yoff+2.5), module, Polyphemus::RADCV_PARAM+j,
            -1,1,0
            ));
 
        addParam(createParam<RoundBlackKnob>(
            Vec(xoff+62.5, yoff-14), module, Polyphemus::RADIUS_PARAM+j,
            -1,1,0
            ));


        addInput(createInput<PJ301MPort>(
            Vec(xoff, yoff+28), module, Polyphemus::ANGLE_INPUT+j
            ));

        addParam(createParam<RoundTinyBlackKnob>(
            Vec(xoff+34, yoff+2.5+28), module, Polyphemus::ANGCV_PARAM+j,
            -1,1,0
            ));
 
        addParam(createParam<RoundBlackKnob>(
            Vec(xoff+62.5, yoff-14+43), module, Polyphemus::ANGLE_PARAM+j,
            0,3.14,0
            ));



    }

    auto* label = new Label();
    label->box.pos=Vec(0, 30);
    label->text = "";
    addChild(label); 
    module->testLabel = label;

    module->ready = 1;

}