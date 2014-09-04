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
define('DRUSH_RECIPES_CORE_COMPATIBILITY', '7');
define('DRUSH_RECIPES_MAX_RECURSION', 5);
define('DRUSH_RECIPES_INVALID', FALSE);
define('DRUSH_RECIPES_VALID', TRUE);
define('DRUSH_RECIPES_DEFAULT_URL', 'http://drush.recipes/recipes.xml');
// listing of the types of drush recipe formats we support
define('DRUSH_RECIPES_FORMAT_REFERENCE', 'reference');
define('DRUSH_RECIPES_FORMAT_TARGET', 'target');
define('DRUSH_RECIPES_FORMAT_MADLIB', 'madlib');
define('DRUSH_RECIPES_FORMAT_CONDITIONAL', 'conditional');
define('DRUSH_RECIPES_FORMAT_CONDITIONAL_QUESTION', 'prompt');
define('DRUSH_RECIPES_FORMAT_ARGUMENT', 'argument');

/**
 * Implements hook_drush_command().
 */
function drush_recipes_drush_command() {
  $items = array();
  $items['drush-list-recipes'] = array(
    'description' => dt('List the available recipes and where they are loaded from'),
    'arguments' => array(
      'list' => dt('Recipies to list, comma separated'),
    ),
    'options' => array(
      'more-detail' => dt('Show all info available'),
      'dr-locations' => dt('User defined list of other locations to load in from'),
    ),
    'aliases' => array('dlr'),
    'examples' => array(
      'drush drush-list-recipes' =>
        'A nicely printed listing of available recipes',
      'drush dlr dr_security,dr_admin_update_status' =>
        'Only list these two recipes',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );
  $items['drush-recipes-url'] = array(
    'description' => dt('Import recipes from a remote service like drush.recipes'),
    'arguments' => array(
      'url' => dt('A URL to return results from.'),
    ),
    'aliases' => array('drurl'),
    'examples' => array(
      'drush drurl ' . DRUSH_RECIPES_DEFAULT_URL . '&uid=10' =>
        'Display a list of recipes for user 10 from the drush.recipes service.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  // @todo determine if we really want to include this here. it seems like
  // the most useful thing this could do is help pull in libraries from the
  // correct destination and put them in the right place in the file structure
  // BUT if that's the case then we should work with the libraries api module
  // to help them fix their commented out, unfinished drush libraries download
  // command!
  /*$items['drush-dl'] = array(
    'description' => dt('Simple download of something'),
    'arguments' => array(
      'url' => dt('A URL to download from.'),
      'desintation' => dt('Destination to save the download'),
      'cache_duration' => dt('How long til we check for a fresh copy, 0 is default'),
    ),
    'aliases' => array('dl'),
    'examples' => array(
      'drush drurl ' . DRUSH_RECIPES_DEFAULT_URL . '&uid=10' =>
        'Display a list of recipes for user 10 from the drush.recipes service.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );*/
  $items['drush-write-recipe'] = array(
    'description' => dt('Interactive prompt to create a new recipe'),
    'aliases' => array('dwr'),
    'examples' => array(
      'drush dwr' =>
        'An interactive prompt will follow',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['drush-validate-recipes'] = array(
    'description' => dt('Validate a saved recipe will run if built from scratch'),
    'aliases' => array('dvr'),
    'examples' => array(
      'drush dvr dr_security' =>
        'Validate that dr_security will work if cooked.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['drush-diff-targets-recipe'] = array(
    'description' => dt('Analyze two targets and create a recipe to chart a course from one to another. Great for figuring out what a client changed after a distro was installed.'),
    'aliases' => array('ddt'),
    'examples' => array(
      'drush ddt @target1 @target2' =>
        'Analysis will occur of how target2 is different then target1 and a suggested recipe to follow how to get target1 to be target2 will be generated (either to the screen or written to a file.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['drush-cook-recipes'] = array(
    'description' => dt('"cook" up a drush recipe'),
    'arguments' => array(
      'recipes' => dt("List of recipes to run, in order and comma separated"),
    ),
    'options' => array(
      'dr-locations' => dt('User defined list of other locations to load in from'),
    ),
    'allow-additional-options' => TRUE,
    'aliases' => array('cook'),
    'examples' => array(
      'drush drush-cook-recipes dr_security' =>
        'Run the dr_security recipe',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
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
    case 'drush:drush-recipes-url':
      return dt('Sync recipes from a remote service like drush.recipes');
    case 'drush:drush-write-recipe':
      return dt('Write a new recipe from commandline');
    case 'drush:drush-diff-targets-recipe':
      return dt('Create a recipe to calculate the diff between systems.');
    case 'drush:drush-cook-recipes':
      return dt('Perform a series of drush recipes');
  /*  case 'drush:drush-dl':
      return dt('Download a file from a url');*/
    case 'drush:drush-validate-recipes':
      return dt('Validate a recipe is in the correct format.');
  }
}

/**
 * Drush command callback for drush-diff-targets-recipe.
 */
function drush_drush_recipes_drush_diff_targets_recipe($source = NULL, $destination = NULL) {
  // test for source being provided by target
  $alias = drush_get_context('DRUSH_TARGET_SITE_ALIAS');
  // if alias is not empty / @none and destination is empty
  if (!empty($alias) && $alias != '@none' && empty($destination)) {
    $destination = $source;
    $source = $alias;
  }
  // test for null case target diffing via lazy method
  if ((empty($destination) || $destination == '@none') && isset($source)) {
    $destination = $source;
    $source = '@none';
  }

  // preflight destination in case it defines the alias used by the source
  _drush_sitealias_get_record($destination);
  // after preflight, get source and destination settings
  $source_settings = drush_sitealias_get_record($source);
  $destination_settings = drush_sitealias_get_record($destination);
  // apply command-specific options.
  drush_sitealias_command_default_options($source_settings, 'source-');
  drush_sitealias_command_default_options($destination_settings, 'target-');
  // select module / themes / profiles in use via system table
  $fields = array('name', 'type', 'status');
  $source_system = _drush_recipes_load_db_table($source_settings, 'system', 'name', $fields);
  $destination_system = _drush_recipes_load_db_table($destination_settings, 'system', 'name', $fields);
  $drush = array();
  // compare the source with the destination
  foreach ($source_system as $project => $data) {
    // ensure A exists on B at least
    if (array_key_exists($project, $destination_system) ) {
      $destination_system[$project]->type = 'found';
      // now check for status
      if ($data->status != $destination_system[$project]->status) {
        // this means source is on, destination is off
        if ($data->status == 1) {
          $drush['dis'][$project] = $project;
        }
        else {
          $drush['en'][$project] = $project;
        }
      }
    }
    else {
      // now we know we need to download something
      $drush['dl'][$project] = $project;
    }
  }
  // test for destination having modules not originally in source
  foreach ($destination_system as $project => $data) {
    // find things we didn't find previously
    if ($data->type != 'found') {
      $drush['dl'][$project] = $project;
      // if enabled then let's mark that it needs enabled
      if ($data->status) {
        $drush['en'][$project] = $project;
      }
    }
  }
  // do a variable comparison
  $source_vars = _drush_recipes_load_db_table($source_settings, 'variable', 'name');
  $destination_vars = _drush_recipes_load_db_table($destination_settings, 'variable', 'name');
  // compare the source with the destination
  foreach ($source_vars as $name => $data) {
    // ensure variable A exists on B
    if (array_key_exists($name, $destination_vars) ) {
      $destination_vars[$name]->name = 'found';
      // now check for status
      if ($data->value != $destination_vars[$name]->value) {
        // this means source is different from destination
        $drush['vset'][$name] = $destination_vars[$name]->value;
      }
    }
    else {
      // now we know we need to delete this variable
      $drush['vdel'][$name] = $name;
    }
  }
  // test for destination having variables not originally in source
  foreach ($destination_vars as $name => $data) {
    // find things we didn't find previously
    if ($data->name != 'found') {
      $drush['vset'][$name] = $data->value;
    }
  }
  // compare permissions and roles
  // load up the source db / roles / permissions
  if ($source_settings['#name'] == 'none') {
    $source_roles = array();
    $source_perms = array();
  }
  else {
    _drush_recipes_db_set_active($source_settings);
    drupal_static_reset('user_role_permissions');
    $source_roles = user_roles();
    $source_perms = user_role_permissions($source_roles);
  }
  // load up the destination db / roles / permissions
  _drush_recipes_db_set_active($destination_settings);
  drupal_static_reset('user_role_permissions');
  $destination_roles = user_roles();
  $destination_perms = user_role_permissions($destination_roles);
  // return to default just to be safe
  db_set_active();

  // comparison, this can be a bit evil..
  foreach ($source_perms as $rid => $permissions) {
    // ensure variable A exists on B
    if (array_key_exists($rid, $destination_perms) ) {
      // dig deeper into permissions arrays
      foreach ($permissions as $permission => $bool) {
        // check that a permission is set
        if (array_key_exists($permission, $destination_perms[$rid]) ) {
          // store the fact that we found this role in some fashion
          $destination_perms[$rid][$permission] = 'found';
        }
        else {
          // this means destination DOES NOT have a perm in source
          $drush['rmp'][$rid][$permission] = TRUE;
        }
      }
      $destination_perms[$rid]['_drush_found'] = 'found';
      // test for destination having permissions not originally in source
      foreach ($destination_perms[$rid] as $permission => $bool) {
        // find things we didn't find previously
        if ($bool != 'found') {
          // a permission exists in destination but not source
          $drush['rap'][$rid][$permission] = TRUE;
        }
      }
    }
    else {
      // a role was deleted
      $drush['rdel'][$rid] = $rid;
    }
  }
  // test for destination having roles not originally in source
  foreach ($destination_perms as $rid => $permissions) {
    // find things we didn't find previously
    if (!isset($permissions['_drush_found'])) {
      // a role exists in destination that's not in source
      $drush['rcrt'][$rid] = $destination_roles[$rid];
      unset($permissions['_drush_found']);
      $drush['rap'][$rid] = $permissions;
    }
  }
  $calls = array();
  // @todo do a features comparison (if applicable)
  //if (module_exists('features')) {
    // drush_features_diff();
    // need to try and get
  //}

  // allow developers working w/ custom comparisons to jump in and do things
  // we may not immediately support in the core plugin; like custom tables
  // or contributed modules that are rare to see in the wild. You could also
  // use this in order to take out calls found above that you want to ignore.
  // Like if you were doing dev to prod target diffing and don't care about
  // permissions differences for the admin user role.
  drush_command_invoke_all_ref('drush_recipes_target_diff_drush_alter', $drush, $source_settings, $destination_settings);
  // now we convert these into drush calls and feed it to its own recipe
  foreach ($drush as $call => $items) {
    switch ($call) {
      // set variables the long way
      case 'vset':
        // optional sanitization by profiler_builder
        if (module_exists('profiler_builder')) {
          $kill = _profiler_builder_ignore_list();
          // unset values we know should never transition
          foreach ($items as $name => $value) {
            if (in_array($name, $kill)) {
              unset($items[$name]);
            }
          }
        }
        else {
          drush_print(dt('While Profiler Builder is not required, it is highly recommended so that it can automatically sanitize variables you don\' want to ship around between builds (like cron keys). run drush dl profiler_builder'));
        }
        // loop through items and build variables with full set statements
        foreach ($items as $name => $value) {
          $calls[] = 'drush ev variable_set("' . $name . '", ' . var_export(unserialize($value), TRUE) . ')';
        }
      break;
      // delete variables
      case 'vdel':
        foreach ($items as $name => $value) {
          $calls[] = 'drush vdel ' . $name;
        }
      break;
      // create roles
      case 'rcrt':
        foreach ($items as $rid => $name) {
          // goofy key names ensures its above permission addition
          $calls['aaa_create_role' . $rid] = 'drush rcrt ' . $rid . ' \'' . $name . '\'';
        }
      break;
      // delete roles
      case 'rdel':
        foreach ($items as $rid) {
          // goofy key names ensures its above permission addition
          $calls[] = 'drush rdel ' . $rid;
        }
      break;
      // add permissions
      case 'rap':
        foreach ($items as $rid => $permissions) {
          foreach ($permissions as $permission => $bool) {
            $calls['zzz_add_perm' . $rid . $permission] = 'drush rap ' . $rid . ' \'' . $permission . '\'';
          }
        }
      break;
      // remove permissions
      case 'rmp':
        foreach ($items as $rid => $permissions) {
          foreach ($permissions as $permission => $bool) {
            $calls[] = 'drush rmp ' . $rid . ' \'' . $permission . '\'';
          }
        }
      break;
      default:
        $calls[$call] = 'drush ' . $call . ' ' . implode(',', $items);
      break;
    }
  }
  // insanely rare use case where we have an exact copy of a site..
  if (!empty($calls)) {
    // switch from easy to write format into arguments
    foreach ($calls as $id => $call) {
      $item = explode(' ', $call);
      // always will be but just be safe
      if ($item[0] == 'drush') {
        array_shift($item);
      }
      $calls[$id] = $item;
    }
    ksort($calls);
    $recipe = array(
      'name' => dt('Diff from @source to @destination', array('@source' => $source, '@destination' => $destination)),
      'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
      'weight' => 0,
      'recipe' => $calls,
      'metadata' => array(
        'type' => 'utility',
        'description' => dt('This is an automatically produced recipe using the DDT command'),
      )
    );
    // list the formats allowed
    $formats = array(
      'json' => 'json',
      'xml' => 'xml'
    );
    // test for yaml since we support that format too
    if (function_exists('yaml_emit')) {
      // allow for writing to file or screen
      $formats['yaml'] = 'yaml';
    }
    $format = drush_choice_with_custom_defaults($formats, 'json', dt('Which recipe encoding?'));
    // allow for writing to file or screen
    $output_locations = array(
      1 => 'write to file',
      2 => 'print',
    );
    // start writing to file if they selected that
    if (drush_choice_with_custom_defaults($output_locations, 'write to file', dt('Write to file or print to screen?')) === 'write to file') {
      $file = $machine_name . '.' . DRUSH_RECIPE_EXTENSION;
      $location = drush_prompt(dt('Location to write recipe'), drush_get_context('DRUSH_PER_USER_CONFIGURATION') . '/drecipes');
      // test directory exists / create it if we can
      if (is_dir("$location") || mkdir("$location")) {
        // try to write recipe to the file
        $output = _drush_recipes_encode($recipe, $format);
        if (!file_put_contents("$location/$file", $output)) {
          drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
          drush_print(_drush_recipes_encode($recipe, $format));
        }
        else {
          // success! tell them how to use this
          drush_log(dt("New recipe written to @file. call `drush cook @machine` to cook up this recipe.", array('@file' => "$location/$file", '@machine' => $machine_name)), 'ok');
        }
      }
      else {
        drush_log(dt("Unable to write folder @location", array('@location' => $location)), 'error');
        drush_print(_drush_recipes_encode($recipe, $format));
      }
    }
    else {
      // just write it to the screen though this is mostly just for debugging
      drush_print(_drush_recipes_encode($recipe, $format));
    }
    return TRUE;
  }
  else {
    drush_print(dt('The sites are exactly the same, most likely something went wrong unless you were testing if they are the same.'));
    return FALSE;
  }
}

/**
 * load table data from a source
 * @return array an array of database info
 */
function _drush_recipes_load_db_table($alias_settings, $table, $key, $fields = '*') {
  // skip out early if this isn't against a target
  if ($alias_settings['#name'] == 'none') {
    return array();
  }
  _drush_recipes_db_set_active($alias_settings);
  $ary = array();
  // selec the fields from the table and convert to an array of values
  $rsc = drush_db_select($table, $fields);
  while ($row = drush_db_fetch_object($rsc)) {
    $ary[$row->{$key}] = $row;
  }
  db_set_active();

  return $ary;
}

/**
 * set active db based on the settings of an alias
 */
function _drush_recipes_db_set_active($alias_settings) {
  // set the context to the alias in question
  drush_sitealias_set_alias_context($alias_settings);
  // get the database credentials from the alias
  sitealias_get_databases_from_record($alias_settings);
  // bootstrap to the max possible for this alias
  drush_bootstrap_max_to_sitealias($alias_settings);
  Database::addConnectionInfo($alias_settings['#name'], 'default', $alias_settings['databases']['default']['default']);
  db_set_active($alias_settings['#name']);
  return 1;
}

/**
 * Drush command callback for drush-recipes-url.
 */
function drush_drush_recipes_url($url = DRUSH_RECIPES_DEFAULT_URL) {
  //pseudo code for now til we build the service out
  $response = file_get_contents($url);
  $contents = utf8_encode($response);
  $recipexml = simplexml_load_string($contents);
  if (!$response->error) {
    $recipe_items = $recipexml->drush_recipe;
    foreach ($recipe_items as $recipes) {
      $recipes = (array) $recipes;
      $location = drush_prompt(dt('Location to write recipes'), drush_get_context('DRUSH_PER_USER_CONFIGURATION') . '/drecipes/_SERVICE-drush.recipes');
      // test directory exists / create it if we can
      if (is_dir("$location") || mkdir("$location")) {
        // loop through recipes that come across
        foreach ($recipes as $recipe) {
          $recipe = (array) $recipe;
          foreach ($recipe as $key => $item) {
            if (gettype($item) == 'object') {
              $recipe[$key] = (array) $item;
            }
            // account for deep nested recipes
            if ($key == 'recipe') {
              foreach ($item as $key2 => $item2) {
                if (gettype($item2) == 'object') {
                  $arytmp = (array) $item2;
                  // see if we need to mash this
                  if (count($arytmp) == 1) {
                    $str = array_pop($arytmp);
                    $recipe[$key][$key2] = explode(' ', $str);
                  }
                }
              }
            }
          }
          // need a file wrapper style call
          // usually these are loaded from the file system
          $objtmp = (object) array('name' => '', 'drecipe' => array());
          $objtmp->name = trim($recipe['machine_name']);
          $objtmp->drecipe = $recipe;
          _drush_recipes_recipe_to_drush($objtmp);
          if (drush_confirm("Are you sure you want to save the above recipe? (this will overwrite your local copy in that directory if its allowed)")) {
            // start writing to file if they selected that
            $file = trim($recipe['machine_name']) . '.' . DRUSH_RECIPE_EXTENSION;
            unset($recipe['machine_name']);
            // validate that this is an actual recipe format
            if (!_drush_recipes_validate_recipe($recipe)) {
              drush_log(dt('The returned recipe message was not a valid recipe, this has been ignored and will not be written to file.'), 'error');
            }
            else {
              // try to write recipe to the file
              $output = _drush_recipes_encode($recipe, 'json');
              if (!file_put_contents("$location/$file", $output)) {
                drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
              }
              else {
                // success! tell them how to use this
                drush_log(dt("New recipe written to @file. call `drush cook @machine` to cook up this recipe.", array('@file' => "$location/$file", '@machine' => $machine_name)), 'ok');
              }
            }
          }
        }
      }
      else {
        drush_log(dt("Unable to write folder @location", array('@location' => $location)), 'error');
      }
    }
  }
  else {
    drush_log(dt("Unable to connect to @url", array('@url' => $url)), 'error');
  }
  return 1;
}

/**
 * Drush command callback for drush-write-recipe.
 */
function drush_drush_recipes_drush_write_recipe() {
  $machine_name = drush_prompt(dt('Enter a machine name'));
  //get the first character of machine name and check if it is a number
  $first_char_machine_name = substr($machine_name, 0, 1);
  if (!ctype_alpha($first_char_machine_name)){
    //echo that input cannot start with a number.
    drush_log('Syntax Error: Please enter a machine name that does not start with a number.', 'error');
    // we return to effectively start a recursive loop but cancel out this one.
    return drush_drush_recipes_drush_write_recipe();
  }
  else {
    $original_machine = $machine_name;
    $machine_name = strtolower($machine_name);
    // check for spaces and replace with underscores.
    $machine_name = preg_replace("/[^a-z0-9_]/" , '_', $machine_name);
    // verify that user wants the converted string; if not start over.
    if (($original_machine != $machine_name) && !drush_confirm(dt('Is @machine the machine name you want?', array('@machine' => $machine_name)))) {
      return drush_drush_recipes_drush_write_recipe();
    }
  }
  $name = drush_prompt(dt('Enter recipe name'));
  // description, version, weight and core are optional
  $description = drush_prompt(dt('Enter a description'), NULL, FALSE);
  $version = drush_prompt(dt('Enter version'), '1.0');
  $weight = drush_prompt(dt('Weight'), '0', FALSE);
  // description and version are optional
  if (!$major_version = drush_drupal_major_version()) {
    $marjor_version = DRUSH_RECIPES_CORE_COMPATIBILITY;
  }
  $core = drush_prompt(dt('Drupal core'), $marjor_version);
  $recipe_types = array(
    1 => 'add-on',
    2 => 'routine',
    3 => 'utility',
  );
  // prompt for the user with the default option
  while (!$type = drush_choice_with_custom_defaults($recipe_types, 'add-on', dt('What type of recipe is this?'), '!value'))
  $type = $recipe_types[$type];
  // author is optional
  $author = drush_prompt(dt('Recipe author'), NULL, FALSE);
  $call = '';
  $items = array();
  while ($call != 'x') {
    $call = drush_prompt(dt('Write the full drush command you want to add (c for conditional, x when done)'));
    // if we didn't get told to stop
    if ($call != 'x') {
      // check for conditional vs complex so we can write these in via the write
      // command
      $item = explode(' ', $call);
      // this means its a reference to an another drecipe
      if (count($item) == 1 && strpos($item[0], '.' . DRUSH_RECIPE_EXTENSION)) {
        $item = (string)$item[0];
      }
      // support referencing make files which we short hand to a drush call
      elseif (count($item) == 1 && strpos($item[0], '.make')) {
        $item = array(
          0 => 'make',
          1 => str_replace('.make', '', $item[0]),
        );
      }
      // support conditionals
      elseif (count($item) == 1 && strpos($item[0], 'c') === 0) {
        $conditions = array();
        $condkey = 0;
        while ($condinput != 'x') {
          $condinput = drush_prompt(dt('CONDITIONAL: Add a drush recipe to add to this conditional (type x when done)'));
          // if we didn't get told to stop and we got a recipe looking entry
          if ($condinput != 'x' && strpos($condinput, '.' . DRUSH_RECIPE_EXTENSION)) {
            // this helps ensure conditions are written starting with key 1
            // can help avoid conflicts with drush options later on
            $condkey++;
            $conditions[$condkey] = (string)$condinput;
          }
          elseif ($condinput == 'x') {
            drush_log(dt('Conditional complete'), 'ok');
          }
          else {
            // ignore input as it wasn't something we understood
            drush_log(dt('Invalid conditional, recipe references only'), 'warning');
            continue;
          }
        }
        // account for no input in case of a mistake
        if (empty($conditions)) {
          continue;
        }
        // add grouping of conditions to the array
        $item = array(
          DRUSH_RECIPES_FORMAT_CONDITIONAL => $conditions,
          DRUSH_RECIPES_FORMAT_CONDITIONAL_QUESTION => drush_prompt(dt("What question are you asking for this conditional?"), 'Which would you like to run?'),
        );
      }
      // easiest case
      elseif ($item[0] == 'drush') {
        array_shift($item);
      }
      else {
        // ignore input as it wasn't something we understood
        drush_log(dt('Invalid command'), 'warning');
        continue;
      }
      // add to array
      $items[] = $item;
    }
  }
  $recipe = array(
    'name' => $name,
    'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
    'weight' => $weight,
    'core' => $core,
    'recipe' => $items,
    'metadata' => array(
      'type' => $type,
      'description' => $description,
      'version' => $version,
      'author' => $author,
    )
  );

  $formats = array(
    'json' => 'json',
    'xml' => 'xml'
  );
  // test for yaml since we support that format too
  if (function_exists('yaml_emit')) {
    // allow for writing to file or screen
    $formats['yaml'] = 'yaml';
  }
  while (!$format = drush_choice_with_custom_defaults($formats, 'json', dt('Which recipe encoding?'), '!value'));
  // allow for writing to file or screen
  $output_locations = array(
    1 => 'write to file',
    2 => 'print',
  );
  // start writing to file if they selected that
  if (drush_choice_with_custom_defaults($output_locations, 'write to file', dt('Write to file or print to screen?')) === 'write to file') {
    $file = $machine_name . '.' . DRUSH_RECIPE_EXTENSION;
    $location = drush_prompt(dt('Location to write recipe'), drush_get_context('DRUSH_PER_USER_CONFIGURATION') . '/drecipes');
    // test directory exists / create it if we can
    if (is_dir("$location") || mkdir("$location")) {
      // try to write recipe to the file
      $output = _drush_recipes_encode($recipe, $format);
      if (!file_put_contents("$location/$file", $output)) {
        drush_log(dt("Unable to write file @file", array('@file' => $file)), 'error');
        drush_print(_drush_recipes_encode($recipe, $format));
      }
      else {
        // success! tell them how to use this
        drush_log(dt("New recipe written to @file. call `drush cook @machine` to cook up this recipe.", array('@file' => "$location/$file", '@machine' => $machine_name)), 'ok');
      }
    }
    else {
      drush_log(dt("Unable to write folder @location", array('@location' => $location)), 'error');
      drush_print(_drush_recipes_encode($recipe, $format));
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
function drush_drush_recipes_drush_list_recipes($list = '') {
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $list = (!empty($list) ? explode(',', $list) : '');
  $rows = array();
  // get option flag for more detail
  $more = drush_get_option('more-detail');
  // make sure we find any
  if (empty($recipes)) {
    drush_log(dt('You have no recipes! Run `drush dwr` to create a new one!'), 'error');
    return FALSE;
  }
  $header = $rows = array();
  // format recipe data as a table array
  foreach ($recipes as $machine_name => $recipe) {
    $row = $recipe->drecipe;
    // clean up data for output
    if ($more) {
      $row['dependencies'] = implode(' ', $row['dependencies']);
      $row['conflicts'] = implode(' ', $row['conflicts']);
      $row['recipe'] = implode("\n", _drush_recipes_recipe_to_drush($row), 0, FALSE, FALSE);
      $row['metadata'] = implode(' ', $row['metadata']);
    }
    else {
      unset($row['dependencies']);
      unset($row['conflicts']);
      unset($row['recipe']);
      unset($row['metadata']);
      unset($row['core']);
      unset($row['drush_recipes_api']);
      unset($row['weight']);
    }
    $row['machine_name'] = $machine_name;
    $row['uri'] = $recipe->filename;
    // allow for limiting what they care about from commandline
    if (!empty($list) && !in_array($machine_name, $list)) {
      continue;
    }
    $rows[] = $row;
  }
  if (empty($rows)) {
    drush_log(dt('No results matched the list you wanted to see'), 'error');
    return FALSE;
  }
  array_unshift($rows, array_keys($row));
  // pretty print the columns
  if ($more) {
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
  }
  else {
    $widths = array();
  }
  drush_print_table($rows, TRUE, $widths);
}

/**
 * Drush command callback for drush-cook-recipes.
 */
function drush_drush_recipes_drush_cook_recipes($list = '', $recurse = FALSE) {
  $commands = array();
  $list = explode(',', $list);
  if (empty($list)) {
    drush_log('You must specify recipes to cook! try running drush dlr for a listing of commands to cook.', 'error');
    return FALSE;
  }
  // allow projects to alter the list
  drush_command_invoke_all_ref('drush_recipes_pre_cook_alter', $list, $recurse);

  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $cook = array_intersect_key($recipes, array_flip($list));
  if (empty($cook)) {
    drush_log('The recipes you have specified don\'t exist, try running drush dlr for a listing of available commands or use --dr-locations to include a location that might have the recipe you just tried to cook.', 'error');
    return FALSE;
  }
  // validate recipes we are going to cook are valid recipe formatted items
  foreach ($cook as $recipe) {
    if (!_drush_recipes_validate_recipe($recipe->drecipe)) {
      // this wasn't a valid format
      return drush_user_abort();
    }
  }
  $dependencies = $conflicts = $weight = array();
  // ensure that they are listed based on weight to form a block chain
  foreach ($cook as $key => $recipe) {
    $weight[$key]  = $recipe->drecipe['weight'];
    $dependencies = array_merge($dependencies,(isset($recipe->drecipe['dependencies']) ? $recipe->drecipe['dependencies'] : array()));
    $conflicts = array_merge($conflicts,(isset($recipe->drecipe['conflicts']) ? $recipe->drecipe['conflicts'] : array()));
  }
  // Sort the data with weight descending
  array_multisort($weight, SORT_ASC, $cook);
  // test for conflicts
  foreach ($conflicts as $conflict) {
    foreach ($cook as $recipe) {
      if ($recipe->name == $conflict) {
        drush_log(dt('Recipes could not be run, @name is listed as a known conflict of one of the included recipes. Review the conflict and try again.', array('@name' => $recipe->name)), 'error');
        return drush_user_abort();
      }
    }
  }
  // ensure dependencies are met
  $notmet = array();
  // build a list of what plugins supply what commands
  $commandlist = drush_get_commands();
  foreach ($commandlist as $command) {
    $drush_projects[] = $command['commandfile'];
  }
  foreach ($dependencies as $dependency) {
    if (drush_is_command($dependency) || in_array($dependency, $drush_projects) || (function_exists('module_exists') && module_exists($dependency)) || (function_exists('module_exists') && $dependency == '@site')) {
      // do nothing, this command exists in some fashion
    }
    else {
      // if we require a working site and didn't get one fail immediately
      if ($dependency == '@site') {
        drush_print(dt('This requires a working drupal site to function!'));
        return drush_user_abort();
      }
      else {
        $notmet[] = $dependency;
      }
    }
  }
  // if anything has failed then we need to tell the user why it didn't run
  if (!empty($notmet)) {
    drush_log(dt("Recipes could not be cooked because the following dependencies have not been met:\n@name\nTry:\ndrush dl @dls\n\nOr did you maybe mean to run this against a site target?", array('@name' => implode("\n", $notmet), '@dls' => implode(',', $notmet))), 'error');
    return drush_user_abort();
  }

  // convert cooking list into commands to run
  drush_print("\n" . dt('Ingredients list to cook: @list', array('@list' => implode(',', $list))));
  $recipe_list = array();
  foreach ($cook as $recipe) {
    $recipe_list[] = _drush_recipes_recipe_to_drush($recipe);
  }
  // confirm cooking as this can be a ton of drush commands
  if ($recurse || drush_confirm('Are you sure you want to cook these recipes with the above call structure? ' . implode(' ', array_keys($cook)))) {
    foreach ($recipe_list as $commands) {
      foreach ($commands as $command) {
        if (is_array($command)) {
          if (isset($command[DRUSH_RECIPES_FORMAT_MADLIB])) {
            drush_log(dt('Adding Madlib ingredient: ') . $command['command'], 'ok');
            // allow for generic overriding of this command
            drush_command_invoke_all_ref('drush_recipes_command_invoke_alter', $command, DRUSH_RECIPES_FORMAT_MADLIB);
            // allow for targetted type overriding
            drush_command_invoke_all_ref('drush_recipes_' . DRUSH_RECIPES_FORMAT_MADLIB . '_command_invoke_alter', $command);
            // ensure process invoking won't fail
            if (!isset($command['target'])) {
              $command['target'] = drush_get_context(DRUSH_TARGET_SITE_ALIAS);
            }
            if (!isset($command['arguments'])) {
              $command['arguments'] = array();
            }
            if (!isset($command['options'])) {
              $command['options'] = array();
            }
            drush_invoke_process($command['target'], $command['command'], $command['arguments'], $command['options']);
            drush_log($command['command'] . dt(' based madlib complete!'), 'ok');
          }
          else if (isset($command[DRUSH_RECIPES_FORMAT_TARGET])) {
            drush_log(dt('Adding ingredient: ') . $command['command'], 'ok');
            // allow for generic overriding of this command
            drush_command_invoke_all_ref('drush_recipes_command_invoke_alter', $command, DRUSH_RECIPES_FORMAT_TARGET);
            // allow for targetted type overriding
            drush_command_invoke_all_ref('drush_recipes_' . DRUSH_RECIPES_FORMAT_TARGET . '_command_invoke_alter', $command);
            // ensure process invoking won't fail
            if (!isset($command['target'])) {
              $command['target'] = drush_get_context(DRUSH_TARGET_SITE_ALIAS);
            }
            if (!isset($command['arguments'])) {
              $command['arguments'] = array();
            }
            if (!isset($command['options'])) {
              $command['options'] = array();
            }
            drush_invoke_process($command['target'], $command['command'], $command['arguments'], $command['options']);
            drush_log($command['command'] . dt(' complete!'), 'ok');
          }
          else if (isset($command[DRUSH_RECIPES_FORMAT_CONDITIONAL])) {
            // shift array keys forward if 0 is set to avoid issues
            if (isset($command[DRUSH_RECIPES_FORMAT_CONDITIONAL][0])) {
              array_unshift($command[DRUSH_RECIPES_FORMAT_CONDITIONAL],'');
              unset($command[DRUSH_RECIPES_FORMAT_CONDITIONAL][0]);
            }
            // force a selection to be made
            while (!$which = drush_choice($command[DRUSH_RECIPES_FORMAT_CONDITIONAL], $command[DRUSH_RECIPES_FORMAT_CONDITIONAL_QUESTION]));
            // load up which based on array position selected from conditional
            $which = $command[DRUSH_RECIPES_FORMAT_CONDITIONAL][$which];
            drush_log(dt('Adding ingredient: ') . $which, 'ok');
            // recursive since they just selected a reference
            drush_drush_recipes_drush_cook_recipes(str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $which), TRUE);
          }
          drush_log($which . dt(' complete!'), 'ok');
        }
        // allow for custom format or else assume argument based
        else if (isset($call['_drush_recipes_custom_format'])) {
          // allow for generic overriding of this command
          drush_command_invoke_all_ref('drush_recipes_command_invoke_alter', $command, $call['_drush_recipes_custom_format']);
          // we put a lot of faith in the developer to know why they just did
          // all this but it will allow for custom call structures beyond those
          // provided by the base plugin
          drush_command_invoke_all_ref('drush_recipes_' . $call['_drush_recipes_custom_format'] . '_command_invoke_alter', $command);
        }
        else {
          drush_log('Adding ingredient: ' . $command, 'ok');
          $command .= ' --y';
          // allow for generic overriding of this command
          drush_command_invoke_all_ref('drush_recipes_command_invoke_alter', $command, DRUSH_RECIPES_FORMAT_ARGUMENT);
          // allow for targetted type overriding
          drush_command_invoke_all_ref('drush_recipes_' . DRUSH_RECIPES_FORMAT_ARGUMENT . '_command_invoke_alter', $command);
          // this is the simple method for running when non interactive
          drush_shell_exec($command);
          $shell_output = drush_shell_exec_output();
          foreach ($shell_output as $shell) {
            drush_print($shell);
          }
          drush_log($command . ' complete!', 'ok');
        }
      }
    }
  }
  else {
    return drush_user_abort();
  }
  // allow projects to react after cooking
  drush_command_invoke_all_ref('drush_recipes_post_cook_alter', $list, $recurse);
}

/**
 * Helper to rebuild drecipe data
 *
 * This is similar to how core handles modules / themes but forked to provide
 * support for modules, themes, and "drecipes" directory structures.
 */
function _drush_recipes_system_rebuild_recipe_data() {
  // // we allow for this command to run regardless of site scope
  $bootstrap = drush_get_context('DRUSH_BOOTSTRAP_PHASE');
  if ($bootstrap != DRUSH_BOOTSTRAP_DRUSH) {
    // test for static cache of output
    $recipes = &drupal_static(__FUNCTION__);
    if (isset($recipes)) {
      return $recipes;
    }
    // find recipes in drush specific locations
    $locations = array(
      'drush-system',
      'drecipes',
      'themes',
      'modules',
    );
    $function_pattern = DRUPAL_PHP_FUNCTION_PATTERN;
  }
  else {
    // we can only check in the drush sub-system for recipes
    $locations = array(
      'drush-system',
    );
    $function_pattern = '[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*';
  }
  // see if the user has defined any additional locations at run time
  if ($dr_locations = drush_get_option('dr-locations', FALSE)) {
    // We have this set with --dr-locations on the cli
    $dr_locations = explode(',', $dr_locations);
    if (is_array($dr_locations)) {
      $locations = array_merge($locations, $dr_locations);
    }
    else {
      $locations[] = $dr_locations;
    }
  }
  // allow drushrc.php options to also be used
  if ($dr_locations = drush_get_option('dr_locations', FALSE)) {
    // We have this set in the drushrc.php file
    if (is_array($dr_locations)) {
      $locations = array_merge($locations, $dr_locations);
    }
    else {
      $locations[] = $dr_locations;
    }
  }

  // rare but check for environmental var
  if (isset($_ENV['DR_LOCATIONS'])) {
    if (is_array($_ENV['DR_LOCATIONS'])) {
      $locations = array_merge($locations, $_ENV['DR_LOCATIONS']);
    }
    else {
      $locations[] = $_ENV['DR_LOCATIONS'];
    }
  }
  // allow projects to inject their own locations
  drush_command_invoke_all_ref('drush_recipes_locations_alter', $locations);

  $recipes = array();
  // file preg_match pattern
  $pattern = '/^' . $function_pattern . '\.' . DRUSH_RECIPE_EXTENSION . '/';
  // loop through each directory and find recipes
  foreach ($locations as $directory) {
    // find recipes in the drecipes directory
    $recipes += _drush_recipes_drupal_system_listing($pattern, $directory, 'name', 0);
  }
  // Set defaults for recipes.
  $defaults = array(
    'drush_recipes_api' => DRUSH_RECIPES_API_VERSION,
    'weight' => 0,
    'core' => DRUSH_RECIPES_CORE_COMPATIBILITY,
    'dependencies' => array(),
    'conflicts' => array(),
    'recipe' => array(),
    'metadata' => array(),
  );

  // Read recipe files for each module/theme/drecipes.
  foreach ($recipes as $key => $recipe) {
    // Look for the info file.
    $recipe->drecipe = _drush_recipes_load_recipe(dirname($recipe->filename) . '/' . $recipe->basename);
    // Skip $recipes that don't provide info or are malformed.
    if (empty($recipe->drecipe) || empty($recipe->drecipe['recipe'])) {
      unset($recipes[$key]);
      continue;
    }
    // Merge in defaults and save.
    $recipes[$key]->drecipe = $recipe->drecipe + $defaults;
    krsort($recipes[$key]->drecipe);
  }
  // allow last minute recipe data modification
  drush_command_invoke_all_ref('drush_recipes_system_recipe_data_alter', $recipes);
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
  $searchdir = array();
  $files = array();
  // drush based paths need to be discovered in a different manner
  if ($directory == 'drush-system') {
    $searchdir[] = drush_get_context('DRUSH_PER_USER_CONFIGURATION');
    $searchdir[] = drush_get_context('DRUSH_SITE_WIDE_CONFIGURATION');
    $searchdir[] = dirname(__FILE__) . '/..';
  }
  elseif (!in_array($directory, array('drush-system', 'drecipes', 'themes', 'modules'))) {
    $searchdir[] = $directory;
  }
  else {
    $searchdir = array($directory);
    $profiles = array();
    // edge case where call is in a drupal directory but drupal isn't installed
    if (function_exists('drupal_get_profile')) {
      $profile = drupal_get_profile();
      // In case both profile directories contain the same extension, the actual
      // profile always has precedence.
      $profiles[] = $profile;
      foreach ($profiles as $profile) {
        if (file_exists("profiles/$profile/$directory")) {
          $searchdir[] = "profiles/$profile/$directory";
        }
      }
    }

    // Always search sites/all/* as well as the global directories.
    $searchdir[] = 'sites/all/' . $directory;
    $config = conf_path();
    if (file_exists("$config/$directory")) {
      $searchdir[] = "$config/$directory";
    }
  }
  foreach ($searchdir as $dir) {
    $files_to_add = drush_scan_directory($dir, $mask, array('.', '..', 'CVS'), 0, TRUE, $key, $min_depth);
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
      if (file_exists($recipe_file = dirname($file->filename) . '/' . $file->basename)) {
        // Get the .DRUSH_RECIPE_EXTENSION file for the module or theme this file belongs to.
        $recipe = _drush_recipes_load_recipe($recipe_file);
        // If the module or theme is incompatible with Drupal core, remove it
        // from the array for the current search directory, so it is not
        // overwritten when merged with the $files array.
        if (isset($recipe['core']) && $recipe['core'] != DRUSH_RECIPES_CORE_COMPATIBILITY) {
          unset($files_to_add[$file_key]);
        }
      }
    }
    $files = array_merge($files, $files_to_add);
  }

  return $files;
}

/**
 * Helper function to load a file.
 *
 * @return array decoded array from one of the formats listed.
 */
function _drush_recipes_load_recipe($path, $validate = FALSE) {
  // if we are attempting a validation we just want to try and load an item
  // back and assert that it is the same as what was passed in
  if ($validate) {
    $contents = $path;
  }
  else {
    // loading as usual, this is a recipe file
    $contents = file_get_contents($path);
    // ensure utf8 to avoid issues
    $contents = utf8_encode($contents);
  }
  // to avoid issues w/ format extension names we trickle down based on support
  $recipe = drush_json_decode($contents);
  // if empty, lets try a yaml parser
  if (empty($recipe) && function_exists('yaml_parse')) {
    // test for yaml capabilities
    $recipe = yaml_parse($contents);
  }
  // lets try xml as a last ditch
  if (empty($recipe)) {
    $recipe = simplexml_load_string($contents);
  }
  // if still empty now we have a REAL problem
  if (empty($recipe)) {
    // something went wrong, we couldn't read the file
    drush_log(dt('There is a problem with the recipe located at @file', array('@file' => $path)), 'error');
    return FALSE;
  }
  drush_command_invoke_all_ref('drush_recipes_after_recipe_loaded_alter', $recipe);
  return $recipe;
}

/**
 * Helper function to export to a certain format.
 *
 * @return array encoded contents for export.
 */
function _drush_recipes_encode($contents, $format = 'json') {
  // allow other advanced tweaks just prior to encoding
  drush_command_invoke_all_ref('drush_recipes_encode_alter', $contents, $format);
  switch ($format) {
    case 'json':
      return _drush_recipes_pretty_json(drush_json_encode($contents));
    break;
    case 'yaml':
      if (function_exists('yaml_emit')) {
        return yaml_emit($contents);
      }
    break;
    case 'xml':
      $xml = new SimpleXMLElement('<drush_recipe/>');
      _drush_recipes_array_to_xml($contents, $xml);
      return $xml->asXML();
    break;
  }
  return FALSE;
}

/**
 * Helper function to convert array to XML
 * @param  array $array any old array
 * @param  object $xml   SimpleXMLElement object type
 * @return object        returns a populated XML object with values from array
 */
function _drush_recipes_array_to_xml($array, &$xml) {
  foreach($array as $key => $value) {
    if(is_array($value)) {
      if(!is_numeric($key)){
        $subnode = $xml->addChild("$key");
        _drush_recipes_array_to_xml($value, $subnode);
      }
      else {
        _drush_recipes_array_to_xml($value, $xml);
      }
    }
    else {
      if(!is_numeric($key)){
        $xml->addChild("$key","$value");
      }
      else {
        $xml->addChild("num$key","$value");
      }
    }
  }
}

/**
 * convert a recipe array to drush calls
 * @param  array $recipe  a recipe file fully loaded with recipe array
 * @param  int   $indent  the level to indent / levels of recursion down
 * @param  bool  $recurse if the functionc all should be recursive
 * @param  bool  $list    if calls should be printed as they are found
 * @return array $drush   array of drush calls in order they should cook
 */
function _drush_recipes_recipe_to_drush($recipe, $indent = 1, $recurse = TRUE, $list = TRUE) {
  drush_command_invoke_all_ref('drush_recipes_to_drush_alter', $recipe);
  // catch to make sure we don't fall into a deep infinite loop of recursive calls
  if ($indent > DRUSH_RECIPES_MAX_RECURSION) {
    // tell people in the event they triggered this and are confused
    drush_log(dt('There is most likely a problem with your logic for this recipe series. It was about to make a call 5 levels down in its referenced call logic, and prevented this from happening to avoid an infinite loop. If you know what you are doing, increase DRUSH_RECIPES_MAX_RECURSION. If you think the current value is too low, please file an issue in the issue queue.'), 'error');
    return array();
  }
  if ($list && $indent === 1) {
    drush_print(_drush_recipes_indent($indent) . '<' . $recipe->name . '>');
  }
  $drush = array();
  foreach ($recipe->drecipe['recipe'] as $call) {
    switch ($format = _drush_recipes_detect_format($call)) {
      // a recipe with a reference to another recipe
      case DRUSH_RECIPES_FORMAT_REFERENCE:
        // we have a recipe nested, lets look up how to build it and recurse
        // but only if told to do so, may just want to reference the command
        if ($recurse) {
          // pull data for full object, fortunately it's static cached
          $recipes = _drush_recipes_system_rebuild_recipe_data();
          // loop through items to cross reference with filename
          foreach ($recipes as $machine_name => $item) {
            // if call matches filename we recurse through that filename's recipe
            if ($machine_name == str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $call)) {
              if ($list) {
                drush_print(_drush_recipes_indent($indent) . '<' . $machine_name . '>');
              }
              $drush += _drush_recipes_recipe_to_drush($item, $indent+1);
              if ($list) {
                drush_print(_drush_recipes_indent($indent) . '</' . $machine_name . '>');
              }
              continue;
            }
          }
        }
        else {
          $print = 'drush der '  . str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $call);
          if ($list) {
            drush_print(_drush_recipes_indent($indent) . $print);
          }
          $drush[] = $print;
        }
      break;
      case DRUSH_RECIPES_FORMAT_MADLIB:
        $drush[] = $call;
        // see if we should print nicely formatted debug stuff
        if ($list) {
          // format the same as target based
          $print = 'MADLIB: drush ' . $call['target'] . ' ' . $call['command'] . ' ' . implode(' ', $call['arguments']);
          if (isset($call['options']) && !empty($call['options'])) {
            // make sure first option gets the dashes it needs
            $print .= _drush_recipes_indent($indent+1) . "\n--" . implode('--', $call['options']);
          }
          // defaults
          if (isset($call['defaults']) && !empty($call['defaults'])) {
            $print .= "\n" . _drush_recipes_indent($indent+1) . dt('defaults: ');
            foreach ($call['defaults'] as $key => $val) {
              // make sure first option gets the dashes it needs
              $print .= "\n" . _drush_recipes_indent($indent+1) . ' ' . $key . ' => ' . $val;
            }
          }
          drush_print(_drush_recipes_indent($indent+1) . $print);
        }
      break;
      case DRUSH_RECIPES_FORMAT_TARGET:
        $drush[] = $call;
        // see if we should print nicely formatted debug stuff
        if ($list) {
          $print = 'drush ' . $call['target'] . ' ' . $call['command'] . ' ' . implode(' ', $call['arguments']);
          if (isset($call['options']) && !empty($call['options'])) {
            // make sure first option gets the dashes it needs
            $print .= '--' . implode(' --', $call['options']);
          }
          drush_print(_drush_recipes_indent($indent+1) . $print);
        }
      break;
      case DRUSH_RECIPES_FORMAT_CONDITIONAL:
        //  conditional is most complex so processing has to happen at run time
        $drush[] = $call;
        if ($list) {
          drush_print(_drush_recipes_indent($indent+1) . 'CONDITIONAL: ' . implode(' OR ', $call[DRUSH_RECIPES_FORMAT_CONDITIONAL]));
        }
      break;
      case DRUSH_RECIPES_FORMAT_ARGUMENT:
        $drush[] = 'drush ' . implode(' ', $call);
        // see if we should print nicely formatted debug stuff
        if ($list) {
          drush_print(_drush_recipes_indent($indent+1) . 'drush ' . implode(' ', $call));
        }
      break;
      default:
        // we didn't pass any of the known recipe specifications
        // this could be because you know what you are doing and are writing
        // your own drush recipes plugin. In the future we'll have an actual
        // plugin architecture but for now we just provide hook support
        drush_command_invoke_all_ref('drush_recipes_to_drush_command_format_alter', $drush, $call, $format, $list);
      break;
    }
  }
  if ($list && $indent === 1) {
    drush_print(_drush_recipes_indent($indent) . '</' . $recipe->name . ">\n");
  }
  return $drush;
}

/**
 * Helper for indenting
 */
function _drush_recipes_indent($count) {
  $output = '';
  $counter = 0;
  while ($counter < $count) {
    $output .= '  ';
    $counter++;
  }
  return $output;
}
/**
 * Print json nicely regardless of PHP version
 */
function _drush_recipes_pretty_json($json) {
  $result = '';
  $level = 0;
  $in_quotes = false;
  $in_escape = false;
  $ends_line_level = NULL;
  $json_length = strlen($json);
  for ($i = 0; $i < $json_length; $i++) {
    $char = $json[$i];
    $new_line_level = NULL;
    $post = "";
    if ($ends_line_level !== NULL) {
      $new_line_level = $ends_line_level;
      $ends_line_level = NULL;
    }
    if ($in_escape) {
      $in_escape = false;
    }
    elseif ($char === '"') {
      $in_quotes = !$in_quotes;
    }
    elseif (!$in_quotes) {
      switch ($char) {
        case '}':
        case ']':
          $level--;
          $ends_line_level = NULL;
          $new_line_level = $level;
        break;

        case '{':
        case '[':
          $level++;
        case ',':
          $ends_line_level = $level;
        break;

        case ':':
          $post = " ";
        break;

        case " ":
        case "\t":
        case "\n":
        case "\r":
          $char = "";
          $ends_line_level = $new_line_level;
          $new_line_level = NULL;
        break;
      }
    }
    elseif ($char === '\\') {
      $in_escape = true;
    }

    if ($new_line_level !== NULL) {
        $result .= "\n".str_repeat("\t", $new_line_level);
    }
    $result .= $char . $post;
  }

  return $result;
}

/**
 * Detect the format of the call.
 */
function _drush_recipes_detect_format($call) {
  // hardest to easiest with exception of it not being an array
  if (!is_array($call)) {
    return DRUSH_RECIPES_FORMAT_REFERENCE;
  }
  else if (isset($call[DRUSH_RECIPES_FORMAT_MADLIB])) {
    return DRUSH_RECIPES_FORMAT_MADLIB;
  }
  else if (isset($call[DRUSH_RECIPES_FORMAT_TARGET])) {
    return DRUSH_RECIPES_FORMAT_TARGET;
  }
  else if (isset($call[DRUSH_RECIPES_FORMAT_CONDITIONAL])) {
    return DRUSH_RECIPES_FORMAT_CONDITIONAL;
  }
  // allow other developers to write their own formats
  drush_command_invoke_all_ref('drush_recipes_detect_format_alter', $call);
  if (isset($call['_drush_recipes_custom_format'])) {
    return $call['_drush_recipes_custom_format'];
  }
  // fallback case is that it's an array of arguments
  return DRUSH_RECIPES_FORMAT_ARGUMENT;
}

/**
 * Implements hook_drush_recipes_FORMAT_command_invoke_alter().
 */
function drush_recipes_drush_recipes_madlib_command_invoke_alter(&$command) {
  $selection = array();
  // allow for falling back if they make a mistake
  $original = $command;
  $print = 'drush ';
  // loop through tokens requesting input from the user
  foreach ($command['tokens'] as $token => $description) {
    $default = NULL;
    $default_set = FALSE;
    // check for option at commandline, this overrides empty AND default values
    // this means you can cook a recipe with these values downsteam by supplying
    // the option at run time: drush cook recipe --mlt-token=value
    if ($default = drush_get_option('mlt-' . str_replace('[', '', str_replace(']', '', $token)))) {
      // stupid but this allows for NULL string setting as the default case
      $default_set = TRUE;
    }
    // see if we have a supplied default if the user is skipping through
    else if (isset($command['defaults'][$token])) {
      $default = $command['defaults'][$token];
      // stupid but this allows for NULL string setting as the default case
      $default_set = TRUE;
    }
    // prompt for input, only requiring it if a default is not set
    $selection[$token] = drush_prompt(dt($description), $default, ($default_set ? FALSE : TRUE));
  }
  // loop through each selected token and scan the target, command, arguments
  // and options (key | value) for matches. None of these are required
  // properties so test each one
  // @todo look for more efficient way of doing this
  foreach ($selection as $token => $value) {
    // replace tokens we find in the target; rare
    if (isset($command['target'])) {
      $command['target'] = str_replace($token, $value, $command['target']);
    }
    // replace tokens we find in the command itself; also rare
    if (isset($command['command'])) {
      $command['command'] = str_replace($token, $value, $command['command']);
    }
    if (isset($command['arguments'])) {
      // loop through the arguments, these only can be matched on the value (arg)
      foreach ($command['arguments'] as $key => $arg) {
        $command['arguments'][$key] = str_replace($token, $value, $arg);
      }
    }
    if (isset($command['options'])) {
      // loop through the options; these can be match on the key OR value OR both
      foreach ($command['options'] as $key => $option) {
        // first the option value, this is the easy part
        $command['options'][$key] = str_replace($token, $value, $option);
        // now the key, a bit more tricky but doable
        $tmp = str_replace($token, $value, $key);
        // check that a replacement was made to the key and that $tmp isn't set
        if ($tmp != $key && !isset($command['options'][$tmp])) {
          // set the new key pair to the value of the old one
          $command['options'][$tmp] = $command['options'][$key];
          // now unset the key as the old key has been token replaced
          unset($command['options'][$key]);
        }
      }
    }
  }
  // build the output for confirmation of command execution

  // target is not required
  if (isset($command['target'])) {
    $print .= $command['target'] . ' ';
  }
  // replace tokens we find in the command itself; rare
  if (isset($command['command'])) {
    $print .= $command['command'] . ' ';
  }
  // tokens can live in arguemnts though this would be rare
  if (isset($command['arguments'])) {
    $print .= implode(' ', $command['arguments']);
  }
  // run through options separately
  if (isset($command['options'])) {
    foreach ($command['options'] as $key => $option) {
      $print .= ' --' . $key . '=' . $option;
    }
  }
  drush_print(dt("You are about to execute this madlib:\n") . $print);
  // confirm decision; if they abort it'll ask the question again
  if (!drush_confirm("\nHit 'y' to execute or 'n' to fill it out again.")) {
    drush_recipes_drush_recipes_madlib_command_invoke_alter($original);
    $command = $original;
  }
}

 /**
 * Replacement callback for drush_choice with better default handling.
 *
 * @param  array $options       an array of input
 * @param  string $default_value the default value if nothing selected
 * @param  string $prompt        question to ask the user
 * @param  string $label         !value, !key or !number for listing
 * @return string                value selected, otherwise $default_value
 */
function drush_choice_with_custom_defaults($options, $default_value, $prompt = 'Enter a number.', $label = '!value'){
  print dt($prompt) . "\n";
  //Make sure array has the correct row and column padding layout.
  $array_pad = 0;
  foreach ($options as $key => $option) {
    if(is_array($option) && (count($option) > $array_pad)){
      $array_pad = count($option);
    }
  }
  //check to see if the $default_value is not set.  If no, populate with cancel
  if(is_null($default_value)){
    $default_value_test = 'Cancel';
  }
  else{
    //else we set the $default_value to the inputted value.
     $default_value_test = $default_value;
  }

  //may have to change the following values to meet standard of array syntax.
  $default_array = array('[0]', ':', $default_value_test);
  $rows[] = array_pad($default_array, $array_pad + 2, '');
  $selection_number = 0;
  foreach ($options as $key => $option) {
    if((substr($key, 0, 3) == '-- ') && (substr($key, -3) == ' --')){
      $rows[] = array_pad(array('', '', $option), $array_pad + 2, '');
    }
    else{
      if($option != $default_value_test){
        $selection_number++;
        $row = array("[$selection_number]", ':');
          if(is_array($option)){
            $row = array_merge($row, $option);
          }
          else{
            $row[] = dt($label, array('!number' => $selection_number, '!key' => $key, '!value' => $option));
          }
        $rows[] = $row;
        $selection_list[$selection_number] = $key;
        }
      }
    }

    foreach ($rows as $row) {
      //gets the intial value of the array.
      if(current($row) == '[0]'){
        for($i = 0; $i < 1; $i++){
         $initial_rows_value = $row[2];
        }
      }
      else{
        //check to see if intital string and current string are equal.
        if(strcmp($initial_rows_value, end($row)) == 0){
          array_splice($rows, 1, -1);
          //if we split, then we have to rebuild the numbering of the array.
        }
      }
    }
    drush_print_table($rows);
    drush_print_pipe(array_keys($options));

    //If user specifies choice (eg choice that isn't default)
    //then make a selection and reprompt if the choice is not an available option
    if(($choice = drush_get_option('choice', FALSE)) !== FALSE){
      //first see if choice is one of the symbolic options
      if(array_key_exists($choice, $options)){
        return $choice;
      }
      elseif(array_key_exists($choice, $selection_list)){
        return $selection_list[$choice];
      }
      //do something here to reprompt
      return FALSE;
    }

    //If the user supplied the default answer
    if(!drush_get_context('DRUSH_NEGATIVE') && !drush_get_context('DRUSH_AFFIRMATIVE') && !drush_get_context('DRUSH_PIPE')){
      while($line = trim(fgets(STDIN))){
        return $selection_list[$line];
      }
    }

    //If only one option in the array
    if(drush_get_context('DRUSH_AFFIRMATIVE') && (count($options) == 1)){
      return $selection_list[0];
    }
    //change this to be the default.
    if($default_value == 'Cancel'){
       drush_print(dt('Cancelled'));
       return FALSE;
    }
    return $default_array[2];
}

/**
 * Drush command callback for drush-validate-recipes.
 */
function drush_drush_recipes_drush_validate_recipes($list = '') {
  $commands = array();
  $list = explode(',', $list);
  if (empty($list)) {
    drush_log('You must specify recipes to validate! try running drush dlr for a listing of commands to test.', 'error');
    return FALSE;
  }
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $validate = array_intersect_key($recipes, array_flip($list));
  if (empty($validate)) {
    drush_log('The recipes you have specified don\'t exist, try running drush dlr for a listing of available commands or use --dr-locations to include a location that might have the recipe you just tried to validate.', 'error');
    return FALSE;
  }
  // validate recipes we are going to cook are valid recipe formatted items
  foreach ($validate as $recipe) {
    if (!_drush_recipes_validate_recipe($recipe->drecipe)) {
      // this wasn't a valid format
      return drush_user_abort();
    }
  }
  return dt('All recipes were valid!');
}

/**
 * Helper function to validate a drush recipe
 */
function _drush_recipes_validate_recipe($recipe) {
  // generic validation that it can be loaded and reloaded from source
  // encode the recipe to json
  $original_recipe = _drush_recipes_encode($recipe, 'json');
  // attempt to load the recipe from json
  $loaded_recipe = _drush_recipes_load_recipe($original_recipe, TRUE);
  // now load again off of this item that was just loaded in
  $new_recipe = _drush_recipes_encode($loaded_recipe, 'json');
  // attempt to load the recipe from json
  $processed_recipe = _drush_recipes_encode(_drush_recipes_load_recipe($new_recipe, TRUE));
  // if this item is not IDENTICAL after being reloaded it must be corrupt
  if ($original_recipe != $processed_recipe) {
     drush_log(dt('Recipe format you attempted to import was corrupt! Check source'), 'error');
     return DRUSH_RECIPES_INVALID;
  }
  // ensure the major api version is set
  if (!isset($recipe['drush_recipes_api'])) {
    drush_log(dt('All recipes must include the `drush_recipes_api` property. Please review drush_recipes.api.php if building a complex recipe by hand.'), 'error');
     return DRUSH_RECIPES_INVALID;
  }
  // check the API of this recipe and validate accordingly
  switch ($recipe['drush_recipes_api']) {
    // in the future we can add additional support for future specs
    case DRUSH_RECIPES_API_VERSION:
      // ensure the required parts of the array are intact
      if (!isset($recipe['name']) || !isset($recipe['recipe'])) {
        drush_log(dt('name and recipe properties are required!'), 'error');
        return DRUSH_RECIPES_INVALID;
      }
      // check for invalid properties, custom stuff belongs in metadata
      // this helps ensure consistency and security
      foreach ($recipe as $key => $value) {
        switch($key) {
          case 'name':
          case 'drush_recipes_api':
          case 'core':
          case 'weight':
          case 'conflicts':
          case 'dependencies':
          case 'recipe':
          case 'metadata':
            // do nothing, these are valid properties in 1.0
          break;
          default:
            drush_log(dt('A property was defined that version @api does not support!', array('@api' => DRUSH_RECIPES_API_VERSION)), 'error');
            return DRUSH_RECIPES_INVALID;
          break;
        }
      }
      // nothing was able to invalidate the recipe so assumption it is safe
      return DRUSH_RECIPES_VALID;
    break;
    default:
      // a specification verison is being defined that we don't know about yet
      // this either signals someone building against their own verison of
      // the spec or that a recipe is corrupt because it has the wrong version
      // number
      drush_log(dt('The supplied API version number of @api is not currently known to the drush recipes plugin. Validation can\'t happen as a result but this is not preventing it from running as you may know why you are doing this (like experimenting with a new API method).', array('@api' => $recipe['drush_recipes_api'])), 'warning');
      return DRUSH_RECIPES_VALID;
    break;
  }
  // we got here, something went wrong
  return DRUSH_RECIPES_INVALID;
}

/**
 * Drush command callback for drush-dl.
 */
/*function drush_drush_recipes_drush_dl($url, $destination, $overwrite = TRUE) {
  // attempt download and saving of file
  if (_drush_download_file($url, $destination, $overwrite)) {
    drush_print(dt('@url successfully saved to @dest', array('@url' => $url, '@dest' => $destination)));
    return TRUE;
  }
  drush_print(dt('Download failed!'));
  return FALSE;
}*/
