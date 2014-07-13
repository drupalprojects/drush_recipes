<?php

/**
 * @file
 * Drush Recipe format.
 *
 * This format allows you to call a series of drush calls to fire in
 * succession. This allows them to be packaged along with modules and
 * themes and is a complementary format to .make files. Where .make is
 * for describing how to make a site. drush recipe files are for producing a
 * structured, reusable, desired result.
 *
 * Much of what you can do with drecipes can be done via features but
 * this obviously is far less packaging cruft. It also allows you to be
 * intentionally destructive with your calls instead of worrying about
 * being in "override hell" with features and features_override.
 */

define('DRUSH_RECIPE_EXTENSION', 'drecipe');
define('DRUSH_RECIPES_API_VERSION', '1.0');
define('DRUSH_RECIPES_FORMAT_DEFAULT', 'json');

/**
 * Implements hook_drush_command().
 */
function drush_recipes_drush_command() {
  $items = array();
  $items['drush-list-recipes'] = array(
    'description' => dt('List the available recipes and where they are loaded from'),
    'arguments' => array(
      'list' => dt("Recipies to list, comma separated"),
    ),
    'aliases' => array('dlr'),
    'examples' => array(
      'drush drush-list-recipes' =>
        'A nicely printed listing of available recipes',
      'drush dlr dr_security,dr_admin_update_status' =>
        'Only list these two recipes',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['drush-write-recipe'] = array(
    'description' => dt('Interactive prompt to create a new recipe'),
    'aliases' => array('dwr'),
    'examples' => array(
      'drush dwr' =>
        'An interactive prompt will follow',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['drush-cook-recipes'] = array(
    'description' => dt('"cook" up a drush recipe'),
    'arguments' => array(
      'recipes' => dt("List of recipes to execute, in order and comma separated"),
    ),
    'aliases' => array('cook'),
    'examples' => array(
      'drush drush-cook-recipes dr_security' =>
        'Run the dr_security recipe',
    ),
  );
  return $items;
}

/**
 * Implements hook_drush_help().
 */
function drush_recipes_drush_help($section) {
  switch ($section) {
    case 'drush:drush-list-recipes':
      return dt('List the available recipes');
    case 'drush:drush-cook-recipes':
      return dt('Perform a series of drush recipes');
    case 'drush:drush-write-recipe':
      return dt('Write a new recipe from commandline');
  }
}

/**
 * Drush command callback for drush-write-recipe.
 */
function drush_drush_recipes_drush_write_recipe() {
  $machine_name = drush_prompt(dt('Enter a machine name'));
  $name = drush_prompt(dt('Enter recipe name'));
  // description and version are optional
  $description = drush_prompt(dt('Enter a description'), '');
  $version = drush_prompt(dt('Enter version'), '1.0');
  $options = array(
    1 => 'add-on',
    2 => 'routine',
  );
  // prompt for type choice then convert to choice name
  $type = drush_choice($options, dt('What type of recipe is this?'));
  $type = $options[$type];
  // author is optional
  $author = drush_prompt(dt('Recipe author'), '');
  $item = '';
  while ($item != 'x') {
    $item = drush_prompt(dt('Write the full drush command you want to add (type x when done)'));
    if ($item != 'x') {
      $ary[] = $item;
    }
  }
  $recipe = array(
    'name' => $name,
    'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
    'weight' => 0,
    'recipe' => $ary,
    'metadata' => array(
      'description' => $description,
      'version' => $version,
      'type' => $type,
      'author' => $author,
    )
  );
  $format = 'json';
  // test for yaml since we support that format too
  if (function_exists('yaml_emit')) {
    // allow for writing to file or screen
    $options = array(
      'json' => 'json',
      'yaml' => 'yaml',
    );
    $format = drush_choice($options, dt('Which recipe encoding?'));
  }
  // allow for writing to file or screen
  $options = array(
    0 => 'print',
    1 => 'write to file',
  );
  // start writing to file if they selected that
  if (drush_choice($options, dt('What type of recipe is this?')) == 1) {
    $file = $machine_name . '.' . DRUSH_RECIPE_EXTENSION;
    $location = drush_prompt(dt('Location to write recipe'), drush_get_context('DRUSH_PER_USER_CONFIGURATION') . '/drecipes');
    // test directory exists / create it if we can
    if (is_dir("$location") || mkdir("$location")) {
      // try to write recipe to the file
      if (!file_put_contents("$location/$file", _drush_recipes_encode($recipe, $format))) {
        drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
      }
      else {
        // success! tell them how to use this
        drush_log(dt("New recipe written to @file. call `drush cook @machine` to cook up this recipe.", array('@file' => "$location/$file", '@machine' => $machine_name)), 'cool!');
      }
    }
    else {
      drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
    }
  }
  else {
    // just write it to the screen though this is mostly just for debugging
    drush_print(_drush_recipes_encode($recipe, $format));
  }
  return TRUE;
}

/**
 * Drush command callback for drush-list-recipes.
 */
function drush_drush_recipes_drush_list_recipes($list = array()) {
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $header = $rows = array();
  //@todo provide support for limiting to list of defined recipes
  // format recipe data as a table array
  foreach ($recipes as $recipe) {
    $row = $recipe->drecipe;
    // clean up data for output
    $row['dependencies'] = implode(' ', $row['dependencies']);
    $row['conflicts'] = implode(' ', $row['conflicts']);
    $row['recipe'] = implode("\n", _drush_recipes_recipe_to_drush($row['recipe']));
    $row['metadata'] = implode(' ', $row['metadata']);
    $row['machine_name'] = $recipe->name;
    $row['uri'] = $recipe->uri;
    $rows[] = $row;
  }
  array_unshift($rows, array_keys($row));
  // pretty print the columns
  $widths = array(
    'uri' => '30',
    'machine_name' => '8',
    'name' => '10',
    'conflicts' => '10',
    'core' => '3',
    'drush_recipes_api' => '3',
    'metadata' => '30',
    'recipe' => '40',
    'dependencies' => '10',
    'weight' => '2',
  );
  drush_print_table($rows, TRUE, $widths);
}

/**
 * Drush command callback for drush-cook-recipes.
 */
function drush_drush_recipes_drush_cook_recipes($list = '') {
  $commands = array();
  $list = explode(',', $list);
  if (empty($list)) {
    drush_log('You must specify recipes to execute! try running drush dlr for a listing of commands', 'error');
    return FALSE;
  }
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $execute = array_intersect_key($recipes, array_flip($list));
  // @todo utilize the recipe weight to ensure correct weighted firing order
  // This should only be used when all defined at once, cookbook style recipes
  // need to treat them sequentially.
  foreach ($execute as $recipe) {
    $commands += _drush_recipes_recipe_to_drush($recipe->drecipe['recipe'], TRUE);
  }
  drush_print(implode("\n", $commands));
  // confirm execution as this can be a ton of drush commands
  if (drush_confirm('Are you sure you want to execute these recipes with the above call structure? ' . implode(' ', array_keys($execute)))) {
    foreach ($commands as $command) {
      $command .= ' --y';
      //@todo execute the recipies via dispatch or drush_invoke not sure which
      drush_log('About to execute: ' . $command, 'ok');
      drush_shell_exec($command);
      $shell_output = drush_shell_exec_output();
      foreach ($shell_output as $shell) {
        drush_print($shell);
      }
      drush_log($command . ' complete!', 'ok');
    }
  }
  else {
    return drush_user_abort();
  }
}

/**
 * Helper to rebuild drecipe data
 *
 * This is similar to how core handles modules / themes but forked to provide
 * support for modules, themes, and "drecipes" directory structures.
 */
function _drush_recipes_system_rebuild_recipe_data() {
  // test for static cache of output
  $recipes = &drupal_static(__FUNCTION__);
  if (isset($recipes)) {
    return $recipes;
  }
  // @todo need to cache this for improved performance in discovery
  $recipes = array();
  // file preg_match pattern
  $pattern = '/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.' . DRUSH_RECIPE_EXTENSION . '/';
  // find recipes in drush specific locations
  $locations = array(
    'drush-system',
    'drecipes',
    'themes',
    'modules'
  );
  // loop through each directory and find recipes
  foreach ($locations as $directory) {
    // find recipes in the drecipes directory
    $recipes += _drush_recipes_drupal_system_listing($pattern, $directory, 'name', 0);
  }

  // Set defaults for recipes.
  $defaults = array(
    'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
    'weight' => 0,
    'core' => DRUPAL_CORE_COMPATIBILITY,
    'dependencies' => array(),
    'conflicts' => array(),
    'recipe' => array(),
    'metadata' => array(),
  );

  // Read recipe files for each module/theme/drecipes.
  foreach ($recipes as $key => $recipe) {

    // Look for the info file.
    $recipe->drecipe = _drush_recipes_load_recipe(dirname($recipe->uri) . '/' . $recipe->name . '.'  . DRUSH_RECIPE_EXTENSION);

    // Skip $recipes that don't provide info or are malformed.
    if (empty($recipe->drecipe) || empty($recipe->drecipe['recipe'])) {
      unset($recipes[$key]);
      continue;
    }

    // Merge in defaults and save.
    $recipes[$key]->drecipe = $recipe->drecipe + $defaults;
    krsort($recipes[$key]->drecipe);
  }

  return $recipes;
}

/**
 * Fork of drupal_system_listing to parse files
 * @param  [type]  $mask      [preg_match for file pattern matching]
 * @param  [type]  $directory [directory to search in]
 * @param  string  $key       [file key]
 * @param  integer $min_depth [depth into the directory to traverse]
 * @return [array]             [an array of objects relating to recipe files]
 */
function _drush_recipes_drupal_system_listing($mask, $directory, $key = 'name', $min_depth = 0) {
  $config = conf_path();

  $searchdir = array();
  $files = array();
  // drush based paths need to be discovered in a different manner
  if ($directory == 'drush-system') {
    $searchdir[] = drush_get_context('DRUSH_PER_USER_CONFIGURATION');
    $searchdir[] = drush_get_context('DRUSH_SITE_WIDE_CONFIGURATION');
    $searchdir[] = dirname(__FILE__) . '/..';
  }
  else {
    $searchdir = array($directory);
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
      // If it has no drecipe file, then we just behave liberally and accept the
      // new resource on the list for merging.
      if (file_exists($recipe_file = dirname($file->uri) . '/' . $file->name . '.' . DRUSH_RECIPE_EXTENSION)) {
        // Get the .DRUSH_RECIPE_EXTENSION file for the module or theme this file belongs to.
        $recipe = _drush_recipes_load_recipe($recipe_file);
        // If the module or theme is incompatible with Drupal core, remove it
        // from the array for the current search directory, so it is not
        // overwritten when merged with the $files array.
        if (isset($recipe['core']) && $recipe['core'] != DRUPAL_CORE_COMPATIBILITY) {
          unset($files_to_add[$file_key]);
        }
      }
    }
    $files = array_merge($files, $files_to_add);
  }

  return $files;
}

/**
 * Helper function to load a file as json or yaml.
 *
 * @return array decoded array.
 */
function _drush_recipes_load_recipe($path, $format = DRUSH_RECIPES_FORMAT_DEFAULT) {
  $contents = file_get_contents($path);
  // ensure utf8 to avoid issues
  $contents = utf8_encode($contents);
  switch ($format) {
    case 'yaml':
      return yaml_parse($contents);
    break;
    case 'json':
      return drush_json_decode($contents);
    break;
  }
  return FALSE;
}

/**
 * Helper function to export to a certain format.
 *
 * @return array encoded contents for export.
 */
function _drush_recipes_encode($contents, $format = DRUSH_RECIPES_FORMAT_DEFAULT) {
  switch ($format) {
    case 'yaml':
      return yaml_emit($contents);
    break;
    case 'json':
      return drush_json_encode($contents);
    break;
  }
  return FALSE;
}

/**
 * convert a recipe array to drush calls
 * @param  [array] $recipe a structured array of drush calls in recipe format
 * @return [array]         array of drush calls in order they should execute
 */
function _drush_recipes_recipe_to_drush($recipe, $recurse = FALSE) {
  $drush = array();
  foreach ($recipe as $call) {
    if (is_array($call)) {
      $drush[] = 'drush ' . implode(' ', $call);
    }
    else {
      // we have a recipe nested, lets look up how to build it and recurse
      // but only if told to do so, may just want to reference the command
      if ($recurse) {
        // pull data for full object, fortunately it's static cached
        $recipes = _drush_recipes_system_rebuild_recipe_data();
        // loop through items to cross reference with filename
        foreach ($recipes as $item) {
          // if call matches filename we recurse through that filename's recipe
          if ($item->filename == $call) {
            $drush += _drush_recipes_recipe_to_drush($item->drecipe['recipe']);
            continue;
          }
        }
        drush_print_r($call);
      }
      else {
        $drush[] = 'drush der '  . str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $call);
      }
    }
  }
  return $drush;
}
