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
  return $form;
}

/**
 * Form submit handler for extra settings
 */
function _drush_recipes_profiler_builder_settings_form_submit($form, &$form_state) {
  // time to get crazy
  // @todo replace this with a user entered form field
  $machine_name = 'auto_recipe.drecipe';
  $files[$machine_name] = drush_recipes_profiler_builder_build_file();
  // Generate download file name
  $filename =  'auto-drecipe.tar';
  // Clear out output buffer to remove any garbage from tar output.
  if (ob_get_level()) {
    ob_end_clean();
  }
  // set headers for file download on submit, accounting for drush
  if (!function_exists('drush_main')) {
    drupal_add_http_header('Content-type', 'application/x-tar');
    drupal_add_http_header('Content-Disposition', 'attachment; filename="' . $filename . '"');
    drupal_send_headers();
  }
  $output = '';
  // loop through file array and add to package
  foreach ($files as $file_name => $file_contents) {
    // write file to tar package
    $output .= profiler_builder_tar_create("{$machine_name}/{$file_name}", $file_contents);
  }
  // end tar file output
  $output .= pack("a1024", "");
  if (!function_exists('drush_main')) {
    print $output;
    exit;
  }
  else {
    // drush is running, return values to drush to handle
    $drush_args = array(
      'tar' => $output,
      'tar_name' => $filename,
      'source_files' => $files,
    );
    return $drush_args;
  }
}
