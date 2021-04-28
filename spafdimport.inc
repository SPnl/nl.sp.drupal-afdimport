<?php

function spafdimport_afdelingen() {

  _spafdimport_output_start('Afdelingen import: ');
  // Get chapter info from procurios.
  $chapters_info_proc = array();
  $selection_id = variable_get('spafdimport_procurios_selection_id');
  $page_nr = 1;
  _spafdimport_output_text('Getting chapters from Procurios');
  while ($result = procapi_selection_get_relations($selection_id, $page_nr)) {
    _spafdimport_output_text('Page ' . $page_nr);
    $page_nr++;
    $chapters_info_proc = array_merge($chapters_info_proc, $result);
  }

  $imported_afdeling_contact_numbers = array();
  if (!empty($chapters_info_proc)) {
    $import_user = spafdimport_get_import_user();
    foreach ($chapters_info_proc as $chapter_info_proc) {
      $node = NULL;
      // Check afdling status.
      $property = '';
      $property_mapping = array (
        9 => 'In Oprichting',
        10 => 'Erkend',
        11 => 'Opgeheven',
        12 => 'Werkgroep',
      );
      if (!empty($chapter_info_proc['properties']) && !empty($chapter_info_proc['contact_number'])) {
        foreach ($chapter_info_proc['properties'] as $property_id => $property_is_set) {
          if ($property_is_set) {
            if (isset($property_mapping[$property_id])) {
              $property = $property_mapping[$property_id];
            }
          }
        }
      }

      if (!empty($property)) {
        // Get afdeling.
        $query = new EntityFieldQuery();
        $query->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', 'afdeling')
          ->fieldCondition('afd_id', 'value', $chapter_info_proc['contact_number']);
        $existing_afdeling = $query->execute();

        // Check if afdeling exists.
        if ( ! empty($existing_afdeling['node'])) {
          $node = reset($existing_afdeling['node']);
          _spafdimport_output_text('Updating afdeling ' . $chapter_info_proc['name']);
        }

        if (empty($node)) {
          // Create new afdeling.
          _spafdimport_output_text('Creating afdeling ' . $chapter_info_proc['name']);
          $node = entity_create('node', array(
            'type'     => 'afdeling',
            'title'    => $chapter_info_proc['name'],
            'status'   => 1,
            'language' => 'nl',
            'uid'      => $import_user->uid,
            'promote'  => 0,
          ));
        }

        // Update values.
        $node_wrapped = entity_metadata_wrapper('node', $node);

        // Get provincie term id.
        $provincie = taxonomy_get_term_by_name($chapter_info_proc['address']['province'], 'provincie');
        if ( ! empty($provincie)) {
          $provincie = reset($provincie);
          $node_wrapped->provincie->set($provincie->tid);
        }
        // Get regio term id.
        $region = taxonomy_get_term_by_name(substr_replace('SP-regio ', '', $chapter_info_proc['region_name'], 'regio'));
        if ( ! empty($region)) {
          $region = reset($region);
          $node_wrapped->regio->set($region->tid);
        }

        $node_wrapped->title  = $chapter_info_proc['name'];
        $node_wrapped->author = $import_user->uid;
        $node_wrapped->link->set(array('url' => $chapter_info_proc['website']));
        $node_wrapped->afd_email->set($chapter_info_proc['email']);
        $node_wrapped->afd_id->set($chapter_info_proc['contact_number']);
        $node_wrapped->afd_status->set($property);
        $node_wrapped->status = 1;
        $node_wrapped->save();
        // Store imported chapter ids for cleanup.
        $imported_afdeling_contact_numbers[$chapter_info_proc['contact_number']] = $chapter_info_proc['contact_number'];
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
      if ( ! in_array($node->afd_id['und'][0]['value'], $imported_afdeling_contact_numbers) && !empty($node->status)) {
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
