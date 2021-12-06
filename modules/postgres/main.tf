resource "yandex_mdb_postgresql_cluster" "replicated_database_instance" {
  name        = var.name
  folder_id   = var.folder_id
  environment = "PRODUCTION"
  network_id  = var.vpc_id

  config {
    version = var.pg_version
    resources {
      resource_preset_id = var.pg_tier_id
      disk_type_id       = "network-ssd"
      disk_size          = var.pg_instance_disk_size
    }
  }

  user {
    name       = var.pg_admin_name
    password   = var.pg_admin_password
    conn_limit = var.pg_admin_conn_limit
    permission {
      database_name = "filestorage"
    }
    permission {
      database_name = "webinar"
    }
    permission {
      database_name = "auth"
    }
    permission {
      database_name = "chat"
    }
    permission {
      database_name = "telegram"
    }
    permission {
      database_name = "sentry"
    }
    permission {
      database_name = "gateway"
    }
    permission {
      database_name = "billing"
    }
  }

  database {
    name  = "default_db"
    owner = var.pg_admin_name
  }

  user {
    name       = var.pg_git_name
    password   = var.pg_git_password
    conn_limit = var.pg_git_conn_limit
  }

  database {
    name  = "git"
    owner = var.pg_git_name
    extension {
      name = "btree_gist"
    }
    extension {
      name = "pg_trgm"
    }
  }
  user {
    name       = var.pg_fs_user_name
    password   = var.pg_fs_user_password
    conn_limit = var.pg_fs_user_conn_limit
  }

  database {
    name       = "filestorage"
    owner      = var.pg_fs_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  user {
    name       = var.pg_web_user_name
    password   = var.pg_web_user_password
    conn_limit = var.pg_web_user_conn_limit
  }

  database {
    name  = "webinar"
    owner = var.pg_web_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  user {
    name       = var.pg_auth_user_name
    password   = var.pg_auth_user_password
    conn_limit = var.pg_auth_user_conn_limit
  }

  database {
    name  = "auth"
    owner = var.pg_auth_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  user {
    name       = var.pg_chat_user_name
    password   = var.pg_chat_user_password
    conn_limit = var.pg_chat_user_conn_limit
  }

  database {
    name  = "chat"
    owner = var.pg_chat_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  user {
    name       = var.pg_sentry_user_name
    password   = var.pg_sentry_user_password
    conn_limit = var.pg_sentry_user_conn_limit
  }

  database {
    name  = "sentry"
    owner = var.pg_sentry_user_name
    extension {
      name = "citext"
    }
  }

  user {
    name       = var.pg_gateway_user_name
    password   = var.pg_gateway_user_password
    conn_limit = var.pg_gateway_user_conn_limit
  }

  database {
    name  = "gateway"
    owner = var.pg_gateway_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  user {
    name       = var.pg_telegram_user_name
    password   = var.pg_telegram_user_password
    conn_limit = var.pg_telegram_user_conn_limit
  }

  database {
    name  = "telegram"
    owner = var.pg_telegram_user_name
    lc_collate = "ru_RU.UTF-8"
    lc_type    = "ru_RU.UTF-8"
  }

  user {
    name       = var.pg_billing_user_name
    password   = var.pg_billing_user_password
    conn_limit = var.pg_billing_user_conn_limit
  }

  database {
    name  = "billing"
    owner = var.pg_billing_user_name
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
