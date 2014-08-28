<?php
  /**
   * Drush Recipes API
   *
   * This documents ways you can modify the behavior of drush recipes as well
   * as how to create your own recipes.
   */

// You can define additional dr_locations to search for in your .drush/drushrc.php
// settings file so that you can stash recipes in a shared location like box.com
// to use this add something like:
// $options['dr_locations'] = '/drushstuff';
// or
// $options['dr_locations'] = array('/drushstuff', '/drecipes');

/**
 * Drush Recipe 1.0
 *
 * name - Human reable name of this recipe; required
 * drush_recipes_api - api version, 1.0 defaults
 * core - drupal core this is compatible with, optional
 * weight - weight relative to other recipes if called in the same block-chain
 *   this defaults to 0
 * dependencies - drush plugin name, drush command, module names, or @site which
 *   means that the command requires a working drupal site to function.
 *   All four are valid dependency types and can be used together but try to
 *   use @site, plugin name or module name when possible, drush command is lazy
 *   and doesn't give good feedback to other developers as to how to meet the
 *   requirement.
 * conflicts - a list of recipes that are known conflicts. Use this if you are
 *   building multiple recipes and you know that they don't play nice together.
 *   This can help save you or others from accidentally trying to run recipes
 *   that do and undo each others functionality (or a security recipe known
 *   to have issues with something that opens up doors).
 * recipe - the structure of commands to execute, this can also be another
 *   recipe filename which will append all the commands in that file ahead of
 *   what is about to execute. There are 4 structures to this listed below
 * metadata - a series of properties that can be used for front-end integration.
 *   this is entirely optional but helps developers understand what you wrote.
 */
$js = <<<JS
{
  "name": "Security defaults",
  "drush_recipes_api": "1.0",
  "weight": 0,
  "core": "7",
  "dependencies": [
    "seckit",
    "paranoia"
  ],
  "conflicts": [
    "insecure_stuff"
  ],
  "recipe": [
    "dr_admin_update_status.drecipe",
    [
      "vset",
      "user_register",
      "1"
    ],
    [
      "dis",
      "php"
    ],
    [
      "pm-uninstall",
      "php"
    ],
    [
      "en",
      "seckit"
    ],
    [
      "en",
      "paranoia"
    ]
  ],
  "metadata": {
    "descrption": "Disable projects that cause issues, increase h-our defenses cap'in!",
    "version": "1.0",
    "type": "add-on",
    "author": "btopro",
    "logo": "files\/image.png"
  }
}
JS>>>;

$js = <<<JS
// here's the 4 major types of calls you can do in a recipe as of the 1.0 spec.
"recipe": [
    "dr_admin_update_status.drecipe", // reference another recipe
    [
      "vset", // each element is an argument passed into a 1 line call w/ drush as prefix
      "user_register",
      "1"
    ],
    [
      "conditional": [ // allow for user to select which sub-routine to run
        "recipe1.drecipe",
        "recipe2.drecipe",
        "recipe3.drecipe"
      ]
    ],
    [// long call format, this allows user to interact w/ the call
    // only use this for complex call structures that you need to interact with
    "target": "cool.d7.site", //target
    "arguments": [ // arguments to pass in, similar to simple structure
      "en",
      "paranoia"
    ],
    "options": ["y"] // any possible options
    ]
  ],
JS>>>;
