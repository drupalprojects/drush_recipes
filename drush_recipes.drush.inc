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
define('DRUSH_RECIPES_CORE_COMPATIBILITY', '7.x');
define('DRUSH_RECIPES_MAX_RECURSION', 5);
define('DRUSH_RECIPES_DEFAULT_URL', 'http://drush.recipes/recipes.xml');
// listing of the types of drush recipe formats we support
define('DRUSH_RECIPES_FORMAT_REFERENCE', 'reference');
define('DRUSH_RECIPES_FORMAT_TARGET', 'target');
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
  $items['drush-write-recipe'] = array(
    'description' => dt('Interactive prompt to create a new recipe'),
    'aliases' => array('dwr'),
    'examples' => array(
      'drush dwr' =>
        'An interactive prompt will follow',
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
      'recipes' => dt("List of recipes to execute, in order and comma separated"),
    ),
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
      return dt('Sync recipes from a remote service');
    case 'drush:drush-write-recipe':
      return dt('Write a new recipe from commandline');
    case 'drush:drush-diff-targets-recipe':
      return dt('Create a recipe to calculate the diff between systems.');
    case 'drush:drush-cook-recipes':
      return dt('Perform a series of drush recipes');
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
  _drush_recipes_db_set_active($source_settings);
  drupal_static_reset('user_role_permissions');
  $source_roles = user_roles();
  $source_perms = user_role_permissions($source_roles);
  _drush_recipes_db_set_active($destination_settings);
  // load up the destination db / roles / permissions
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
        'description' => 'This is an automatically produced recipe',
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
  /*$response = file_get_contents($url);
  $contents = utf8_encode($response);
  $recipes = simplexml_load_string($contents);
  drush_print_r((array) $recipes->drush_recipe);
  if (!$response->error) {
    $location = drush_prompt(dt('Location to write recipes'), drush_get_context('DRUSH_PER_USER_CONFIGURATION') . '/drecipes/service-drush.recipes/');
    // test directory exists / create it if we can
    if (is_dir("$location") || mkdir("$location")) {
      foreach ($recipes as $recipe) {
        // should list the recipe so we know what we are saving from remote
        // @todo probably need to wrap recipe in a file wrapper / object
        _drush_recipes_recipe_to_drush($recipe);
        if (drush_confirm("Are you sure you want to save the above recipe? (this will overwrite your local copy in that directory if its allowed)")) {
          // start writing to file if they selected that
          $file = $recipe->name . '.' . DRUSH_RECIPE_EXTENSION;
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
    else {
      drush_log(dt("Unable to write folder @location", array('@location' => $location)), 'error');
    }
  }
  else {
    drush_log(dt("Unable to connect to @url", array('@url' => $url)), 'error');
  }*/
  drush_print(dt('Coming soon'));
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
  // description and version are optional
  $description = drush_prompt(dt('Enter a description'), NULL, FALSE);
  $version = drush_prompt(dt('Enter version'), '1.0');
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
    'weight' => 0,
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
    drush_log('You must specify recipes to cook! try running drush dlr for a listing of commands', 'error');
    return FALSE;
  }
  $recipes = _drush_recipes_system_rebuild_recipe_data();
  $cook = array_intersect_key($recipes, array_flip($list));
  $dependencies = $conflicts = array();
  // ensure that they are listed based on weight to form a block chain
  foreach ($cook as $key => $recipe) {
    $weight[$key]  = $recipe->drecipe['weight'];
    array_merge($dependencies,(isset($recipe->drecipe['dependencies']) ? $recipe->drecipe['dependencies'] : array()));
    array_merge($conflicts,(isset($recipe->drecipe['conflicts']) ? $recipe->drecipe['conflicts'] : array()));
  }
  // Sort the data with weight descending
  array_multisort($weight, SORT_ASC, $cook);
  // test for conflicts
  foreach ($conflicts as $conflict) {
    foreach ($cook as $recipe) {
      if ($recipe->name == $conflict) {
        drush_log(dt('Recipes could not be run, @name is listed as a known conflict of one of the included recipes. Review the conflict and try again.', array('@name' => $recipe->name)), 'error');
        drush_user_abort();
      }
    }
  }
  // ensure dependencies are met
  /*foreach ($dependencies as $dependency) {
    // @todo test for listed dependencies using drush functions somehow
    // dependencies can be both drupal modules as well as drush plugins
    // in the case of drush RR registry_rebuild is required
    if () {
      drush_log(dt('Recipes could not be run, @name is listed as a known conflict of one of the included recipes. Review the conflict and try again.', array('@name' => $recipe->name)), 'error');
      drush_user_abort();
    }
  }*/

  // convert cooking list into commands to execute
  drush_print("\n" . dt('Ingredients list to cook: @list', array('@list' => implode(',', $list))));
  $recipe_list = array();
  foreach ($cook as $recipe) {
    $recipe_list[] = _drush_recipes_recipe_to_drush($recipe);
  }
  // confirm cooking as this can be a ton of drush commands
  if ($recurse || drush_confirm('Are you sure you want to execute these recipes with the above call structure? ' . implode(' ', array_keys($cook)))) {
    foreach ($recipe_list as $commands) {
      foreach ($commands as $command) {
        if (is_array($command)) {
          // test for complex process invoking
          if (isset($command[DRUSH_RECIPES_FORMAT_TARGET])) {
            drush_log('Adding ingredient: ' . $command['command'], 'ok');
            drush_invoke_process($command[DRUSH_RECIPES_FORMAT_TARGET], $command['command'], $command['args'], $command['options']);
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
            drush_log('Adding ingredient: ' . $which, 'ok');
            // recursive since they just selected a reference
            drush_drush_recipes_drush_cook_recipes(str_replace('.' . DRUSH_RECIPE_EXTENSION, '', $which), TRUE);
          }
        }
        else {
          drush_log('Adding ingredient: ' . $command, 'ok');
          $command .= ' --y';
          // this is the simple method for running when non interactive
          drush_shell_exec($command);
          $shell_output = drush_shell_exec_output();
          foreach ($shell_output as $shell) {
            drush_print($shell);
          }
        }
        drush_log($command . ' complete!', 'ok');
      }
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
function _drush_recipes_load_recipe($path) {
  $contents = file_get_contents($path);
  // ensure utf8 to avoid issues
  $contents = utf8_encode($contents);
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
  return $recipe;
}

/**
 * Helper function to export to a certain format.
 *
 * @return array encoded contents for export.
 */
function _drush_recipes_encode($contents, $format = 'json') {
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
 * @param  bool  $recurse if the functionc all should be recursive
 * @param  bool  $list    if calls should be printed as they are found
 * @return array $drush   array of drush calls in order they should execute
 */
function _drush_recipes_recipe_to_drush($recipe, $indent = 1, $recurse = TRUE, $list = TRUE) {
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
      case DRUSH_RECIPES_FORMAT_TARGET:
        $drush[] = $call;
        // see if we should print nicely formatted debug stuff
        if ($list) {
          $print = 'drush ' . $call['target'] . implode(' ', $call['arguments']);
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
  // test for most complex
  if (isset($call[DRUSH_RECIPES_FORMAT_TARGET])) {
    return DRUSH_RECIPES_FORMAT_TARGET;
  }
  else if (isset($call[DRUSH_RECIPES_FORMAT_CONDITIONAL])) {
    return DRUSH_RECIPES_FORMAT_CONDITIONAL;
  }
  // fallback case is that it's an array of arguments
  return DRUSH_RECIPES_FORMAT_ARGUMENT;
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
