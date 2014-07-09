<?php

/**
 * @file
 * Drush Recipe format.
 *
 * This format allows you to call a series of drush calls to fire in
 * succession. This allows them to be packaged along with modules and
 * themes and is a complementary format to .make files. Where .make is
 * for describing how to make a site. .drecipe are for producing a
 * structured, reusable, desired result.
 *
 * Much of what you can do with drecipes can be done via features but
 * this obviously is far less packaging cruft. It also allows you to be
 * intentionally destructive with your calls instead of worrying about
 * being in "override hell" with features and features_override.
 */

/**
 * Implements hook_drush_command().
 */
function drush_recipe_drush_command() {
  $items = array();
  $items['drush-list-recipes'] = array(
    'description' => dt('List the available recipes and where they are loaded from'),
    'aliases' => array('dlr'),
    'examples' => array(
      'drush drush-list-recipes' =>
        'A nicely printed listing of available recipes',
    ),
  );
  $items['drush-execute-recipes'] = array(
    'description' => dt('execute a drush recipe'),
    'arguments' => array(
      'recipes' => dt("List of recipes to execute, in order and comma separated"),
    ),
    'aliases' => array('der'),
    'examples' => array(
      'drush drush-execute-recipes security-harden' =>
        'Run the security-harden recipe',
    ),
  );
  return $items;
}

/**
 * Implements hook_drush_help().
 */
function drush_recipe_drush_help($section) {
  switch ($section) {
    case 'drush:drush-list-recipes':
      return dt('List the available recipes');
    case 'drush:drush-execute-recipes':
      return dt('Perform a series of drush recipes');
  }
}

/**
 * Drush command callback for drush-list-recipes.
 */
function drush_recipe_drush_list_recipes() {
  $recipes = _drush_recipe_system_rebuild_recipe_data();
  // format recipe data as a table array
  foreach ($recipes as $recipe) {
    $rows[] = $recipe;
  }
  // load the headers
  $header = array_keys($recipes[0]);
  drush_print_table($rows, $header);
}

/**
 * Drush command callback for drush-execute-recipes.
 */
function drush_recipe_drush_execute_recipes($recipes = array()) {
  //@todo execute the recipies via dispatch or drush_invoke not sure which
  drush_dispatch($command, $arguments);
}

/**
 * Helper to rebuild drecipe info data
 */
function _drush_recipe_system_rebuild_recipe_data() {
  // Find recipes in modules directories
  $recipes[] = _drush_recipe_drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.drecipe$/', 'modules', 'name', 0);
  // Find recipes in themes directories
  $recipes[] = _drush_recipe_drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.drecipe$/', 'themes', 'name', 0);
  // find recipes in the cookbooks directory
  $recipes[] = _drush_recipe_drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.drecipe$/', 'cookbooks', 'name', 0);

  // Set defaults for recipe info.
  $defaults = array(
    'dependencies' => array(),
    'version' => 1,
    'files' => array(),
  );

  // Read info files for each module/theme/cookbook.
  foreach ($recipes as $key => $recipe) {

    // Look for the info file.
    $recipe->info = drupal_parse_info_file(dirname($recipe->uri) . '/' . $recipe->name . '.drecipe');

    // Skip $recipes that don't provide info.
    if (empty($recipe->info)) {
      unset($recipes[$key]);
      continue;
    }

    // Merge in defaults and save.
    $recipes[$key]->info = $recipe->info + $defaults;
  }

  return $recipes;
}

/**
 * Fork of drupal_system_listing to parse .drecipe files
 * @param  [type]  $mask      [description]
 * @param  [type]  $directory [description]
 * @param  string  $key       [description]
 * @param  integer $min_depth [description]
 * @return [type]             [description]
 */
function _drush_recipe_drupal_system_listing($mask, $directory, $key = 'name', $min_depth = 1) {
  $config = conf_path();

  $searchdir = array($directory);
  $files = array();

  // The 'profiles' directory contains pristine collections of modules and
  // themes as organized by a distribution. It is pristine in the same way
  // that /modules is pristine for core; users should avoid changing anything
  // there in favor of sites/all or sites/<domain> directories.
  $profiles = array();
  $profile = drupal_get_profile();
  // In case both profile directories contain the same extension, the actual
  // profile always has precedence.
  $profiles[] = $profile;
  foreach ($profiles as $profile) {
    if (file_exists("profiles/$profile/$directory")) {
      $searchdir[] = "profiles/$profile/$directory";
    }
  }

  // Always search sites/all/* as well as the global directories.
  $searchdir[] = 'sites/all/' . $directory;

  if (file_exists("$config/$directory")) {
    $searchdir[] = "$config/$directory";
  }

  // Get current list of items.
  if (!function_exists('file_scan_directory')) {
    require_once DRUPAL_ROOT . '/includes/file.inc';
  }
  foreach ($searchdir as $dir) {
    $files_to_add = file_scan_directory($dir, $mask, array('key' => $key, 'min_depth' => $min_depth));

    // Duplicate files found in later search directories take precedence over
    // earlier ones, so we want them to overwrite keys in our resulting
    // $files array.
    // The exception to this is if the later file is from a module or theme not
    // compatible with Drupal core. This may occur during upgrades of Drupal
    // core when new modules exist in core while older contrib modules with the
    // same name exist in a directory such as sites/all/modules/.
    foreach (array_intersect_key($files_to_add, $files) as $file_key => $file) {
      // If it has no info file, then we just behave liberally and accept the
      // new resource on the list for merging.
      if (file_exists($info_file = dirname($file->uri) . '/' . $file->name . '.drecipe')) {
        // Get the .drecipe file for the module or theme this file belongs to.
        $info = drupal_parse_info_file($info_file);

        // If the module or theme is incompatible with Drupal core, remove it
        // from the array for the current search directory, so it is not
        // overwritten when merged with the $files array.
        if (isset($info['core']) && $info['core'] != DRUPAL_CORE_COMPATIBILITY) {
          unset($files_to_add[$file_key]);
        }
      }
    }
    $files = array_merge($files, $files_to_add);
  }

  return $files;
}


