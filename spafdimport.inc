<?php

function spafdimport_afdelingen() {

  set_time_limit(0);

  $civiapi     = spciviapi_get_api();
  $import_user = spafdimport_get_import_user();

  _spafdimport_output_start('Afdelingen import: ');

  // Get tags and custom field id
  $civiapi->Tag->Get(array(
  ));


  $tags_raw = $civiapi->result()->values;
  if (!empty($tags_raw)) {
    foreach ($tags_raw as $tag) {
      if (in_array($tag->name, array("Erkend", "In Oprichting", "Opgeheven", "Werkgroep"))) {
        $tags[$tag->name] = $tag->id;
      }
    }
  }

  $civiapi->CustomField->Getsingle(array(
    'name' => 'Provincie',
    'column_name' => 'provincie',
    'return' => 'id',
  ));
  $cf_prov = 'custom_' . $civiapi->result()->id;

  $civiapi->CustomField->Getsingle(array(
    'name' => 'Regio',
    'column_name' => 'regio',
    'return' => 'id',
  ));
  $cf_regio = 'custom_' . $civiapi->result()->id;

  // Get import data.
  $civiapi->Contact->Get(array(
    'contact_type'          => 'Organization',
    'contact_sub_type'      => 'SP_afdeling',
    'tag'                   => implode(',',$tags),
    'option.limit'          => 9999,
    'return'                => implode(',',array('email', 'contact_id','display_name',$cf_prov, $cf_regio)),
  ));
  $afdelingen = $civiapi->result()->values;

  // Get provinces
  $civiapi->Contact->Get(array(
    'contact_type'          => 'Organization',
    'contact_sub_type'      => 'SP_provincie',
    'option.limit'          => 9999,
    'return'                => implode(',',array('contact_id','display_name')),
  ));
  $prov = $civiapi->result()->values;
  $provinces = array(); 
  if (!empty($prov)) {
    foreach($prov as $p) {
      $provinces[$p->contact_id] = str_replace(array('SP-provincie '), '', $p->display_name);
    }
  }

  // Get regios
  $civiapi->Contact->Get(array(
    'contact_type'          => 'Organization',
    'contact_sub_type'      => 'SP_Regio',
    'option.limit'          => 9999,
    'return'                => implode(',',array('contact_id','display_name')),
  ));
  $regios_raw = $civiapi->result()->values;
  $regios = array(); 
  if (!empty($regios_raw)) {
    foreach($regios_raw as $r) {
      $regios[$r->contact_id] = str_replace(array('SP-regio '), '', $r->display_name);
    }
  }

  $new_afdelingsnames = array();
  if (!empty($afdelingen)) {
    foreach ($afdelingen as $afdeling) {
      // Show progress.
      _spafdimport_output_progress();

      // Clean up name
      $naam = $afdeling->display_name;
      $naam = trim(str_ireplace(['SP-afdeling', 'SP afdeling', 'afdeling'], '', $naam));

      // Filter partijbureau.
      if ($naam == 'SP Partijbureau') {
        continue;
      }

      // Get website
      $civiapi->Website->Get(array(
        'contact_id' => $afdeling->contact_id,
      ));
      $websites = $civiapi->result();
      if ($websites && $websites->count > 0)
        $website = array_shift($websites->values)->url;
      else
        $website = '';

      // Get province name
      if (array_key_exists($afdeling->$cf_prov, $provinces))
        $provname = $provinces[$afdeling->$cf_prov];
      else
        $provname = '';
      // echo "$naam $website " . ($provname ? $provname : "<strong>NO PROVINCE</strong>") . "...<br />\n";

      // Get regio name
      if (array_key_exists($afdeling->$cf_regio, $regios))
        $regionname = $regios[$afdeling->$cf_regio];
      else
        $regionname = '';


      // Get status.
      $status = '';
      $civiapi->EntityTag->get(array(
        'entity_id' => $afdeling->id,
      ));

      if (!$civiapi->result()->is_error) {
        $tags_selected = $civiapi->result()->values;
        foreach ($tags_selected as $tag) {
          if (in_array($tag->tag_id, $tags)) {
            $status = array_search($tag->tag_id, $tags);
            break;
          }
        }
      }
      else {
        $status = 'civierror';
      }

      // Rest van het script is zoals de afdimport-module in SPwebteam/sp-website:

      // Get afdeling.
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'afdeling')
        ->propertyCondition('title', $naam);
      $existing_afdelingen = $query->execute();

      // Check if afdeling exists.
      if ( ! empty($existing_afdelingen['node'])) {
        // Update existing afdeling with current values.
        _spafdimport_output_text('Updating afdeling ' . $naam);
        $node = reset($existing_afdelingen['node']);
      }
      else {
        // Create new afdeling.
        _spafdimport_output_text('Creating afdeling ' . $naam);
        $node = entity_create('node', array(
          'type'     => 'afdeling',
          'title'    => $naam,
          'status'   => 1,
          'language' => 'nl',
          'uid'      => $import_user->uid,
          'promote'  => 0,
        ));
      }
      // Update values.
      $node_wrapped         = entity_metadata_wrapper('node', $node);
      $node_wrapped->title  = $naam;
      $node_wrapped->author = $import_user->uid;
      $node_wrapped->link->set(array('url' => $website));
      // Set afdeling email.
      if (isset($node_wrapped->afd_email)) {
        $node_wrapped->afd_email->set($afdeling->email);
      }
      // Set afdeling id.
      if (isset($node_wrapped->afd_id)) {
        $node_wrapped->afd_id->set($afdeling->id);
      }
      // Get provincie term id.
      $provincie = taxonomy_get_term_by_name($provname, 'provincie');
      if ( ! empty($provincie)) {
        $provincie = reset($provincie);
        $node_wrapped->provincie->set($provincie->tid);
      }
      // Get regio term id.
      $region = taxonomy_get_term_by_name($regionname, 'regio');
      if ( ! empty($region)) {
        $region = reset($region);
        $node_wrapped->regio->set($region->tid);
      }
      if ($status != 'civierror') {
        $node_wrapped->afd_status->set($status);
      }
      $node_wrapped->status = 1;
      $node_wrapped->save();
      // Store Afdelingsnaam for cleanup.
      $new_afdelingsnames[] = $naam;
    }
  }
  // Cleanup afdelingen that do not exist anymore.
  _spafdimport_output_text("\n");
  $query = new EntityFieldQuery();
  // Get current afdelingen.
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'afdeling');
  $afdelingen = $query->execute();
  foreach ($afdelingen['node'] as $afdeling) {
    $node = node_load($afdeling->nid);
    if ( ! in_array($node->title, $new_afdelingsnames)) {
      // Unpublish afdeling.
      _spafdimport_output_text('Unpublishing afdeling ' . $node->title);
      $node->status = 0;
      node_save($node);
    }
    else {
      // Rebuild node permissions.
      node_access_acquire_grants($node);
    }
  }
  _spafdimport_output_finish();
}

function spafdimport_get_import_user() {
  if ( ! ($user = user_load_by_name('import_user'))) {
    $password = user_password(8);
    $fields   = array(
      'name'   => 'import_user',
      'mail'   => 'import_user@example.com',
      'pass'   => $password,
      'status' => 1,
      'init'   => 'Email',
      'roles'  => array(
        DRUPAL_AUTHENTICATED_RID => 'authenticated user',
      ),
    );

    //the first parameter is left blank so a new user is created
    $user = user_save(null, $fields);
  }

  return $user;
}

function _spafdimport_output_text($text) {
  if (function_exists('drush_print'))
    drush_print($text);
  else
    echo($text . "\n");
}

function _spafdimport_output_start($label) {
  if (function_exists('drush_print'))
    drush_print($label . ' ', 0, null, false);
  else
    echo($label . ' ');
}

function _spafdimport_output_progress() {
  if (function_exists('drush_print'))
    drush_print('.', 0, null, false);
}

function _spafdimport_output_finish() {
  if (function_exists('drush_print'))
    drush_print('', 0, null, true);
}
