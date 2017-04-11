/**
 * @file lv_slider.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_conf.h"
#if USE_LV_SLIDER != 0

#include "lv_slider.h"
#include "misc/math/math_base.h"
#include "../lv_draw/lv_draw.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool lv_slider_design(lv_obj_t * slider, const area_t * mask, lv_design_mode_t mode);
static void lv_sliders_init(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_sliders_t lv_sliders_def;	/*Default slider style*/

static lv_design_f_t ancestor_design_f;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*----------------- 
 * Create function
 *-----------------*/

/**
 * Create a slider objects
 * @param par pointer to an object, it will be the parent of the new slider
 * @param copy pointer to a slider object, if not NULL then the new object will be copied from it
 * @return pointer to the created slider
 */
lv_obj_t * lv_slider_create(lv_obj_t * par, lv_obj_t * copy)
{
    /*Create the ancestor slider*/
	/*TODO modify it to the ancestor create function */
    lv_obj_t * new_slider = lv_bar_create(par, copy);
    dm_assert(new_slider);
    
    /*Allocate the slider type specific extended data*/
    lv_slider_ext_t * ext = lv_obj_alloc_ext(new_slider, sizeof(lv_slider_ext_t));
    dm_assert(ext);

    /*Initialize the allocated 'ext' */
    ext->cb = NULL;
    ext->tmp_value = ext->bar.min_value;

    /* Save the bar design function.
     * It will be used in the sllider design function*/
    if(ancestor_design_f == NULL) ancestor_design_f = lv_obj_get_design_f(new_slider);

    /*The signal and design functions are not copied so set them here*/
    lv_obj_set_signal_f(new_slider, lv_slider_signal);
    lv_obj_set_design_f(new_slider, lv_slider_design);

    /*Init the new slider slider*/
    if(copy == NULL) {
        lv_obj_set_style(new_slider, lv_sliders_get(LV_SLIDERS_DEF, NULL));
        lv_obj_set_click(new_slider, true);
    }
    /*Copy an existing slider*/
    else {
    	lv_slider_ext_t * copy_ext = lv_obj_get_ext(copy);

        /*Refresh the style with new signal function*/
        lv_obj_refr_style(new_slider);
    }
    
    return new_slider;
}

/**
 * Signal function of the slider
 * @param slider pointer to a slider object
 * @param sign a signal type from lv_signal_t enum
 * @param param pointer to a signal specific variable
 * @return true: the object is still valid (not deleted), false: the object become invalid
 */
bool lv_slider_signal(lv_obj_t * slider, lv_signal_t sign, void * param)
{
    bool valid;

    /* Include the ancient signal function */
    /* TODO update it to the ancestor's signal function*/
    valid = lv_bar_signal(slider, sign, param);

    /* The object can be deleted so check its validity and then
     * make the object specific signal handling */
    if(valid != false) {
        lv_slider_ext_t * ext = lv_obj_get_ext(slider);
       // lv_bars_t * style = lv_obj_get_style(slider);
        point_t p;
        cord_t w = lv_obj_get_width(slider);
        cord_t h = lv_obj_get_height(slider);
        int16_t tmp;

        switch(sign) {
            case LV_SIGNAL_PRESSED:
                ext->tmp_value = lv_bar_get_value(slider);
                break;
            case LV_SIGNAL_PRESSING:
                lv_dispi_get_point(param, &p);
                if(w > h) {
                    p.x -= slider->cords.x1 + h / 2;    /*Modify the point to shift with half knob (important on the start and end)*/
                    tmp = (int32_t) ((int32_t) p.x * (ext->bar.max_value - ext->bar.min_value + 1)) / (w - h);
                } else {
                    p.y -= slider->cords.y1 + w / 2;    /*Modify the point to shift with half knob (important on the start and end)*/
                    tmp = (int32_t) ((int32_t) p.y * (ext->bar.max_value - ext->bar.min_value + 1)) / (h - w);
                    tmp = ext->bar.max_value - tmp;     /*Invert he value: small value means higher y*/
                }

                lv_bar_set_value(slider, tmp);
                break;

            case LV_SIGNAL_PRESS_LOST:
                lv_bar_set_value(slider, ext->tmp_value);
                break;
            case LV_SIGNAL_RELEASED:
                ext->tmp_value = lv_bar_get_value(slider);
                lv_bar_set_value(slider, ext->tmp_value);
                if(ext->cb != NULL) ext->cb(slider, param);
                break;
            default:
                break;
        }
    }

    return valid;
}

/*=====================
 * Setter functions
 *====================*/

/**
 * Set a function which will be called when a new value is set on the slider
 * @param slider pointer to slider object
 * @param cb a callback function
 */
void lv_slider_set_action(lv_obj_t * slider, lv_action_t cb)
{
    lv_slider_ext_t * ext = lv_obj_get_ext(slider);
    ext->cb = cb;
}


/*=====================
 * Getter functions
 *====================*/

/*
 * New object specific "get" function comes here
 */


/**
 * Return with a pointer to a built-in style and/or copy it to a variable
 * @param style a style name from lv_sliders_builtin_t enum
 * @param copy copy the style to this variable. (NULL if unused)
 * @return pointer to an lv_sliders_t style
 */
lv_sliders_t * lv_sliders_get(lv_sliders_builtin_t style, lv_sliders_t * copy)
{
	static bool style_inited = false;

	/*Make the style initialization if it is not done yet*/
	if(style_inited == false) {
		lv_sliders_init();
		style_inited = true;
	}

	lv_sliders_t  *style_p;

	switch(style) {
		case LV_SLIDERS_DEF:
			style_p = &lv_sliders_def;
			break;
		default:
			style_p = &lv_sliders_def;
	}

	if(copy != NULL) memcpy(copy, style_p, sizeof(lv_sliders_t));

	return style_p;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


/**
 * Handle the drawing related tasks of the sliders
 * @param slider pointer to an object
 * @param mask the object will be drawn only in this area
 * @param mode LV_DESIGN_COVER_CHK: only check if the object fully covers the 'mask_p' area
 *                                  (return 'true' if yes)
 *             LV_DESIGN_DRAW: draw the object (always return 'true')
 *             LV_DESIGN_DRAW_POST: drawing after every children are drawn
 * @param return true/false, depends on 'mode'
 */
static bool lv_slider_design(lv_obj_t * slider, const area_t * mask, lv_design_mode_t mode)
{
    /*Return false if the object is not covers the mask_p area*/
    if(mode == LV_DESIGN_COVER_CHK) {
    	return ancestor_design_f(slider, mask, mode);
    }
    /*Draw the object*/
    else if(mode == LV_DESIGN_DRAW_MAIN) {

        lv_slider_ext_t * ext = lv_obj_get_ext(slider);


        cord_t w = lv_obj_get_width(slider);
        cord_t h = lv_obj_get_height(slider);

        /*Modify the bar act_value to keep until the farer edge of knob*/
        int16_t tmp;
        int16_t range = ext->bar.max_value - ext->bar.min_value;
        if(w >= h) {
            int16_t knob_value = (int32_t)((int32_t)h * range) / w;
            tmp = (int32_t)((int32_t)(range - (ext->bar.act_value - ext->bar.min_value)) * knob_value) / range;
            ext->bar.act_value +=tmp;
        } else {
            int16_t knob_value = (int32_t)((int32_t)w * range) / h;
            tmp = (int32_t)((int32_t)(range - (ext->bar.act_value - ext->bar.min_value)) * knob_value) / range;
            ext->bar.act_value +=tmp;

        }

        ancestor_design_f(slider, mask, mode);
        ext->bar.act_value -=tmp;
        lv_sliders_t * style = lv_obj_get_style(slider);
        area_t knob_area;
        area_cpy(&knob_area, &slider->cords);

        if(w >= h) {
            knob_area.x2 = (int32_t) ((int32_t)(w - h) * ext->bar.act_value) / range;
            knob_area.x2 += knob_area.x1;
            knob_area.x2 += h;
            knob_area.x1 = knob_area.x2 - h;
        } else {

            knob_area.y1 = (int32_t) ((int32_t)(h - w) * ext->bar.act_value) / range;
            knob_area.y1 = knob_area.y2 - knob_area.y1;
            knob_area.y1 -= w;
            knob_area.y2 = knob_area.y1 + w;
        }

        lv_draw_rect(&knob_area, mask, &style->knob);

    }
    /*Post draw when the children are drawn*/
    else if(mode == LV_DESIGN_DRAW_POST) {

    }

    return true;
}


/**
 * Initialize the built-in slider styles
 */
static void lv_sliders_init(void)
{
	/*Default style*/
    lv_bars_get(LV_BARS_DEF, &lv_sliders_def.bar);
    lv_sliders_def.bar.indic.radius = LV_RECT_CIRCLE;
    lv_sliders_def.bar.bg.radius = LV_RECT_CIRCLE;
    lv_rects_get(LV_RECTS_PLAIN, &lv_sliders_def.knob);
    lv_sliders_def.knob.base.color = COLOR_SILVER;
    lv_sliders_def.knob.gcolor = COLOR_GRAY;
    lv_sliders_def.knob.base.opa = OPA_70;
    lv_sliders_def.knob.radius = lv_sliders_def.bar.indic.radius;

}

#endif
