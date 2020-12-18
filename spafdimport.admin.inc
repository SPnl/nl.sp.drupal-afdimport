<?php

/**
 * Define admin form.
 */
function spafdimport_settings_form($form, $form_state) {

  $selections_list = procapi_selection_get_select_list();

  $form['spafdimport_procurios_selection_id'] = array(
    '#type' => 'select',
    '#title' => 'Kies de selectie in Procurios met alle afdelingen',
    '#options' => $selections_list,
    '#default_value' => variable_get('spafdimport_procurios_selection_id'),
  );

  return system_settings_form($form);
}
