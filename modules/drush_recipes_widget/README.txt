This offers a form element called drush_recipes_widget which can be used to render options from the server. this is useful when integrating drupal request systems like AEgir, devshop or ELMSLN with the capabilities of jenkins, bash, and general automation.

<?php
$form['recipes'] = array(
  '#type' => 'drush_recipes_widget',
  '#title' => t('Recipies'),
  '#recipe_type' => 'add-ons',
);
?>
