<?php
/**
 * @file
 * Administrative forms
 */

/**
 * Implements hook_settings().
 */
function _drush_recipes_profiler_builder_settings() {
  $form = array();
  $form['drush_recipes'] = array(
    '#description' => t('Some more crazy things you can do to extend profiler builder'),
    '#collapsed' => FALSE,
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#title' => t('Extra options'),
  );
  // make sure this is enabled
  if (_profiler_builder_extras_query_enabled()) {
    $form['buttons']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
      '#weight' => 10,
      '#submit' => array('_drush_recipes_profiler_builder_settings_form_submit'),
    );
  }
  else {
    $form['recipes'] = array(
      '#type' => 'markup',
      '#value' => t('You need to activate profiler builder @recording', array('@recording' => l('Profiler builder extras', 'admin/config/development/profiler_builder'))),
    );
  }
  // special debugging handler if devel is also installed, common
  if (module_exists('devel')) {
    $form['buttons']['devel_submit'] = array(
      '#type' => 'submit',
      '#value' => t('Review queries logged'),
      '#weight' => 11,
      '#submit' => array('_profiler_builder_extras_settings_devel_form_submit'),
    );
  }
  return $form;
}

/**
 * Form submit handler for extra settings
 */
function _drush_recipes_profiler_builder_settings_form_submit($form, &$form_state) {
  // time to get crazy
  $files = _profiler_builder_extras_load_query_files(FALSE);
  foreach ($files as $queries) {
    foreach ($queries as $query) {
      dpm($query);
    }
  }
  drupal_set_message('Settings saved.');
}

/**
 * Form submit handler for viewing the current queries in the queue to be written
 */
function _profiler_builder_extras_settings_devel_form_submit($form, &$form_state) {
  drupal_set_message('The follow queries are what is planned to be written to your profile');
  dpm(_profiler_builder_extras_load_query_files(FALSE));
}
