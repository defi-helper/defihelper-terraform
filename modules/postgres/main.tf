resource "yandex_mdb_postgresql_cluster" "postgresql_cluster" {
  name        = "${var.name}-cluster"
  folder_id   = var.folder_id
  environment = "PRODUCTION"
  network_id  = var.vpc_id

  config {
    version = var.pg_version
    resources {
      resource_preset_id = var.pg_tier_id
      disk_type_id       = var.enable_replication ? "local-ssd" : "network-ssd"
      disk_size          = var.pg_instance_disk_size
    }
  }

  user {
    name       = var.pg_admin_name
    password   = var.pg_admin_password
    conn_limit = var.pg_admin_conn_limit
    permission {
      database_name = "defihelper"
    }
    permission {
      database_name = "scanner"
    }
    permission {
      database_name = "adapters"
    }
  }

  database {
    name  = "default_db"
    owner = var.pg_admin_name
  }

  user {
    name       = var.pg_defihelper_user_name
    password   = var.pg_defihelper_user_password
    conn_limit = var.pg_defihelper_user_conn_limit
  }

  database {
    name  = "defihelper"
    owner = var.pg_defihelper_user_name
    lc_collate = "en_US.UTF-8"
    lc_type    = "en_US.UTF-8"
  }

  user {
    name       = var.pg_scanner_user_name
    password   = var.pg_scanner_user_password
    conn_limit = var.pg_scanner_user_conn_limit
  }

  database {
    name       = "scanner"
    owner      = var.pg_scanner_user_name
    lc_collate = "en_US.UTF-8"
    lc_type    = "en_US.UTF-8"
  }

  user {
    name       = var.pg_adapters_user_name
    password   = var.pg_adapters_user_password
    conn_limit = var.pg_adapters_user_conn_limit
  }

  database {
    name       = "adapters"
    owner      = var.pg_adapters_user_name
    lc_collate = "en_US.UTF-8"
    lc_type    = "en_US.UTF-8"
  }

  user {
    name       = var.pg_open_user_name
    password   = var.pg_open_user_password
    conn_limit = var.pg_open_user_conn_limit
  }

  database {
    name       = "open"
    owner      = var.pg_open_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  dynamic "host" {
    for_each = var.enable_replication ? [for conf in var.location_subnets : {
      zone      = conf.zone
      subnet_id = conf.id
    }] : [{
      zone      = var.location_subnets[0].zone
      subnet_id = var.location_subnets[0].id
    }]

    content {
      zone             = host.value.zone
      subnet_id        = host.value.subnet_id
      assign_public_ip = var.pg_public_ip
    }
  }
}
