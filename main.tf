provider "yandex" {
  token     = var.yandex_token
  cloud_id  = var.yandex_cloud_id
  folder_id = var.yandex_folder_id
}

locals {
  cluster_service_account_name      = "${var.cluster_name}-cluster"
  cluster_node_service_account_name = "${var.cluster_name}-node"
  container_registry_sa_name        = "${var.cluster_name}-cr"

  cluster_node_group_configs = var.cluster_node_group_configs
  cluster_node_groups = {
    for key, config in local.cluster_node_group_configs :
    key => merge(config, {
      fixed_scale = lookup(var.node_groups_scale[key], "fixed_scale", false) != false ? [var.node_groups_scale[key].fixed_scale] : []
      auto_scale  = lookup(var.node_groups_scale[key], "auto_scale", false) != false ? [var.node_groups_scale[key].auto_scale] : []
      regional = var.node_groups_scale[key].regional
      node_group_label = var.node_groups_scale[key].node_group_label
      node_group_taint = lookup(var.node_groups_scale[key], "node_group_taint", false) != false ? [var.node_groups_scale[key].node_group_taint] : []
      subnet_num = var.node_groups_scale[key].subnet_num
    })
  }
  node_selectors = {
    for key, id in module.cluster.node_group_ids :
    key => {
      #"yandex.cloud/node-group-id" = id
      "group_name" = key
    }
  }
  hosts = {
    dashboard = {
      name   = "k8s"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    kibana = {
      name   = "kb"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    alertmanager = {
      name   = "alertmanager"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    prometheus = {
      name   = "prometheus"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    grafana = {
      name   = "grafana-k8s"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    kiali = {
      name   = "kiali"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    zipkin = {
      name   = "zipkin"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
    sentry = {
      name   = "sentry"
      issuer = module.cert-manager.cluster_issuers["production"]
    }
  }
  ingress = {
    for key, host in local.hosts :
    key => merge(host, { domain = "${host.name}.${var.cluster_domain}" })
  }
#  elasticsearch_username = keys(module.elasticsearch.elasticsearch_user)[0]
#  elasticsearch_password = module.elasticsearch.elasticsearch_user[local.elasticsearch_username]
#  elasticsearch_url      = "http://${local.elasticsearch_username}:${local.elasticsearch_password}@${module.elasticsearch.elasticsearch_host}:9200"
}

module "vpc" {
  source = "./modules/vpc"

  name      = var.cluster_name
  folder_id = var.yandex_folder_id
}

module "iam" {
  source = "./modules/iam"

  cluster_folder_id                 = var.yandex_folder_id
  cluster_service_account_name      = local.cluster_service_account_name
  cluster_node_service_account_name = local.cluster_node_service_account_name
  container_registry_sa_name        = local.container_registry_sa_name
}

module "cluster" {
  source = "./modules/cluster"

  name                       = var.cluster_name
  public                     = true
  kube_version               = var.cluster_version
  release_channel            = var.cluster_release_channel
  vpc_id                     = module.vpc.vpc_id
  location_subnets           = module.vpc.location_subnets
  cluster_service_account_id = module.iam.cluster_service_account_id
  node_service_account_id    = module.iam.cluster_node_service_account_id
  cluster_node_groups        = local.cluster_node_groups
  ssh_keys                   = module.admins.ssh_keys
  enable_replication         = var.enable_replication
  dep = [
    module.iam.req
  ]
}

provider "helm" {
  debug = true

  kubernetes {
    host                   = module.cluster.external_v4_endpoint
    cluster_ca_certificate = module.cluster.ca_certificate

    #config_path = var.kubeconfig_path

    #load_config_file = false
    exec {
      api_version = "client.authentication.k8s.io/v1beta1"
      args        = ["managed-kubernetes", "create-token", "--cloud-id", var.yandex_cloud_id, "--folder-id", var.yandex_folder_id, "--token", var.yandex_token]
      command     = "${path.root}/yc-cli/bin/yc"
    }
  }
}

provider "kubernetes" {
  host                   = module.cluster.external_v4_endpoint
  cluster_ca_certificate = module.cluster.ca_certificate

  #config_path = var.kubeconfig_path

  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    args        = ["managed-kubernetes", "create-token", "--cloud-id", var.yandex_cloud_id, "--folder-id", var.yandex_folder_id, "--token", var.yandex_token]
    command     = "${path.root}/yc-cli/bin/yc"
  }
}

module "nginx-ingress" {
  source           = "./modules/ingress-nginx"
  node_selector    = local.node_selectors["web"]
  load_balancer_ip = var.load_balancer_ip
  nginx_ingress_replicacount = var.nginx_ingress_replicacount
}

provider "kubectl" {
  host                   = module.cluster.external_v4_endpoint
  cluster_ca_certificate = module.cluster.ca_certificate

  #config_path = var.kubeconfig_path

  #load_config_file = false
  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    args        = ["managed-kubernetes", "create-token", "--cloud-id", var.yandex_cloud_id, "--folder-id", var.yandex_folder_id, "--token", var.yandex_token]
    command     = "${path.root}/yc-cli/bin/yc"
  }
}

provider "http" {}

module "cert-manager" {
  source = "./modules/cert-manager"

  issuers_email = var.admin_email

  node_selector = local.node_selectors["service"]

}

module "kubernetes-dashboard" {
  source = "./modules/kubernetes-dashboard"

  node_selector = local.node_selectors["service"]

  ingress = local.ingress["dashboard"]
}

module "admins" {
  source = "./modules/admins"

  admins           = var.admins
  cluster_name     = var.cluster_name
  cluster_endpoint = module.cluster.external_v4_endpoint
}

provider "local" {}

provider "random" {}

/*
module "nfs-server-provisioner" {
  source = "./modules/nfs-server-provisioner"

  node_selector = local.node_selectors["nfs"]
  storage_class = "yc-network-ssd-nonreplicated"
  storage_size  = var.nfs_disk_size
}
*/

module "registry" {
  source = "./modules/registry"

  registry_name = var.cluster_name
}

# module "elasticsearch" {
#   source = "./modules/elasticsearch"
#
#   cluster_name   = var.cluster_name
#   node_selector  = local.node_selectors["logs"]
#   scale          = lookup(var.node_groups_scale["service"], "fixed_scale", 3)
#   storage_class  = "yc-network-ssd"
#   storage_size   = "50Gi"
#   kibana_ingress = local.ingress["kibana"]
# }

module "prometheus" {
  source = "./modules/prometheus"

  configs = {
    alertmanager = {
      ingress       = local.ingress["alertmanager"]
      node_selector = local.node_selectors["service"]
      storage_class = "yc-network-ssd"
      storage_mode  = "ReadWriteOnce"
      storage_size  = "10Gi"
    }
    grafana = {
      ingress   = local.ingress["grafana"]
      node_selector = local.node_selectors["service"]
      http_auth = false
    }
    operator = {
      node_selector = local.node_selectors["service"]
    }
    prometheus = {
      node_selector = local.node_selectors["service"]
      ingress       = local.ingress["prometheus"]
      storage_class = "yc-network-ssd"
      storage_mode  = "ReadWriteOnce"
      storage_size  = "10Gi"
    }
    adapter = {
      node_selector = local.node_selectors["service"]
    }
  }
  grafana_admin_password = var.grafana_admin_password
  prometheus_auth        = var.prometheus_auth
  alertmanager_email_from     = var.alertmanager_email_from
  alertmanager_email_to       = var.alertmanager_email_to
  alertmanager_smtp_address    = var.alertmanager_smtp_address
  alertmanager_smtp_password  = var.alertmanager_smtp_password
  telegram_token = var.telegram_token
  telegram_chat_id = var.telegram_chat_id
}

module "redis" {
  source = "./modules/redis"

  redis_name       = "${var.cluster_name}-redis"
  vpc_id           = module.vpc.vpc_id
  location_subnets = module.vpc.location_subnets
  enable_replication     = var.enable_replication
}

module "bastion" {
  source = "./modules/bastion"

  subnet_id  = module.vpc.location_subnets[0].id
  ip_address = cidrhost(module.vpc.location_subnets[0].v4_cidr_blocks[0], 127)
  name       = var.cluster_name
  zone       = module.vpc.location_subnets[0].zone

  bastion_nat_ip_address            = var.bastion_nat_ip_address
  bastion_core_fractions            = var.bastion_core_fractions
  bastion_cores                     = var.bastion_cores
  bastion_memory                    = var.bastion_memory
  bastion_allow_stopping_for_update = var.bastion_allow_stopping_for_update
  bastion_ssh_users_file_path       = var.bastion_ssh_users_file_path
  bastion_vm_image                  = "fd80mrhj8fl2oe87o4e1"
}

module "postgres" {
  source                 = "./modules/postgres"
  name                   = "${var.cluster_name}-postgres"
  folder_id              = var.yandex_folder_id
  vpc_id                 = module.vpc.vpc_id
  pg_version             = var.pg_version
  pg_tier_id             = var.pg_tier_id
  pg_instance_disk_size  = var.pg_instance_disk_size
  pg_admin_name          = var.pg_admin_name
  pg_admin_password      = var.pg_admin_password
  pg_admin_conn_limit    = var.pg_admin_conn_limit
  pg_defihelper_user_name            = var.pg_defihelper_user_name
  pg_defihelper_user_password        = var.pg_defihelper_user_password
  pg_defihelper_user_conn_limit      = var.pg_defihelper_user_conn_limit
  pg_scanner_user_name        = var.pg_scanner_user_name
  pg_scanner_user_password    = var.pg_scanner_user_password
  pg_scanner_user_conn_limit  = var.pg_scanner_user_conn_limit
  enable_replication     = var.enable_replication
  location_subnets       = module.vpc.location_subnets
  pg_public_ip           = var.pg_public_ip
}

/*
module "mysql" {
  source                   = "./modules/mysql"
  name                     = "${var.cluster_name}-mysql"
  vpc_id                   = module.vpc.vpc_id
  mysql_version            = var.mysql_version
  mysql_tier_id            = var.mysql_tier_id
  mysql_instance_disk_size = var.mysql_instance_disk_size
  mysql_admin_name         = var.mysql_admin_name
  mysql_admin_password     = var.mysql_admin_password
  mysql_admin_conn_limit   = var.mysql_admin_conn_limit
  mysql_antitreningi_name         = var.mysql_antitreningi_name
  mysql_antitreningi_password     = var.mysql_antitreningi_password
  mysql_antitreningi_conn_limit   = var.mysql_antitreningi_conn_limit
  mysql_oplatakursov_name         = var.mysql_oplatakursov_name
  mysql_oplatakursov_password     = var.mysql_oplatakursov_password
  mysql_oplatakursov_conn_limit   = var.mysql_oplatakursov_conn_limit
  enable_replication       = var.enable_replication
  location_subnets         = module.vpc.location_subnets
}

module "dns" {
  source                   = "./modules/dns"
  name                     = "${var.cluster_name}-dns"
  vpc_id                   = module.vpc.vpc_id
  dns_zones                = var.dns_zones
  dns_zones_rs             = var.dns_zones_rs
}
*/

# resource "yandex_iam_service_account" "gitlabstorage" {
#   folder_id   = var.yandex_folder_id
#   name        = "gitlabstorage"
#   description = "service account gitlabstorage"
# }

# // Grant permissions
# resource "yandex_resourcemanager_folder_iam_member" "gitlabstorage-editor" {
#   folder_id = var.yandex_folder_id
#   role      = "storage.editor"
#   member    = "serviceAccount:${yandex_iam_service_account.gitlabstorage.id}"
# }

# // Create Static Access Keys
# resource "yandex_iam_service_account_static_access_key" "gitlabstorage-static-key" {
#   service_account_id = yandex_iam_service_account.gitlabstorage.id
#   description        = "static access key for object storage"
# }

# resource "yandex_storage_bucket" "gitlabstorage" {
#   bucket = "gitlabstorage"

#   access_key = yandex_iam_service_account_static_access_key.gitlabstorage-static-key.access_key
#   secret_key = yandex_iam_service_account_static_access_key.gitlabstorage-static-key.secret_key
# }
/*
module "gitlab" {
  enable_gitlab                  = var.enable_gitlab
  source                         = "./modules/gitlab"
  node_selector                  = local.node_selectors["service"]
  cluster_domain                 = var.cluster_domain
  gitlab_psql_host               = var.gitlab_psql_host
  gitlab_psql_port               = var.gitlab_psql_port
  pg_git_password                = var.pg_git_password
  pg_git_name                    = var.pg_git_name
  gitlab_redis_host              = var.gitlab_redis_host
  gitlab_redis_port              = var.gitlab_redis_port
  gitlab_email_from              = var.gitlab_email_from
  gitlab_smtp_address            = var.gitlab_smtp_address
  gitlab_smtp_port               = var.gitlab_smtp_port
  gitlab_certificate_secret_name = var.gitlab_certificate_secret_name
  ssl_1iu_ru_crt                 = var.ssl_1iu_ru_crt
  ssl_1iu_ru_key                 = var.ssl_1iu_ru_key
  gitlabstorage-static-key       = var.gitlabstorage-static-key
  backups_cron_git               = var.backups_cron_git
  gitlab_smtp_user_name          = var.gitlab_smtp_user_name
  gitlab_smtp_password           = var.gitlab_smtp_password
  git-s3cfg                      = var.git-s3cfg
  gitlab-backup-storage          = var.gitlab-backup-storage
  gitlab-tmp-storage             = var.gitlab-tmp-storage
  version_gitlab                 = var.version_gitlab
  dep                            = [module.cert-manager.cluster_issuers["production"]]
}
*/

module "gitlab-runner" {
  source                        = "./modules/gitlab-runner"
  enable_gitlab_runner          = var.enable_gitlab_runner
  cluster_domain                = var.cluster_domain
  gitlabRunnerRegistrationToken = var.gitlabRunnerRegistrationToken
  gitlab_runner_docker_io_auth  = var.gitlab_runner_docker_io_auth
  gitlab_runner_tags            = var.gitlab_runner_tags
}
/*
module "rabbitmq" {
  source                = "./modules/rabbit"
  rabbitmq_host         = var.rabbitmq_host
  cluster_domain        = var.cluster_domain
  rabbitmq_password     = var.rabbitmq_password
  rabbitmq_erlangcookie = var.rabbitmq_erlangcookie
  rabbitmq_replicaCount = var.rabbitmq_replicaCount
  dep            = [
    module.cert-manager.cluster_issuers["production"],
  ]
}
*/

module "loki-stack" {
  source                = "./modules/loki-stack"
  loki_bucket_name = var.loki_bucket_name
  loki_bucket_access_key = module.s3.s3_loki_static_access_key
  loki_bucket_secret_key = module.s3.s3_loki_static_secret_key
  dep = [
    module.s3
  ]
}

/*
module "swagger" {
  source         = "./modules/swagger"
  ssl_1iu_ru_crt = var.ssl_1iu_ru_crt
  ssl_1iu_ru_key = var.ssl_1iu_ru_key
  cluster_domain = var.cluster_domain
  dep            = [
    module.cert-manager.cluster_issuers["production"],
    module.nginx-ingress,
  ]
}
*/

module "pgadmin4" {
  source                  = "./modules/pgadmin4"
  node_selector           = local.node_selectors["service"]
  cluster_domain          = var.cluster_domain
  pgadmin4_admin_password = var.pgadmin4_admin_password
  pgadmin4_admin_email    = var.pgadmin4_admin_email
  pgadmin4_domain         = var.pgadmin4_domain
  dep            = [
    module.cert-manager.cluster_issuers["production"],
    module.nginx-ingress,
  ]
}

/*
module "phpmyadmin" {
  source         = "./modules/phpmyadmin"
  node_selector  = local.node_selectors["service"]
  cluster_domain = var.cluster_domain
  ssl_1iu_ru_crt = var.ssl_1iu_ru_crt
  ssl_1iu_ru_key = var.ssl_1iu_ru_key
  dep            = [
    module.cert-manager.cluster_issuers["production"],
    module.nginx-ingress,
  ]
}

module "istio" {
  source           = "./modules/istio"
  node_selector    = local.node_selectors["service"]
  cluster_domain   = var.cluster_domain
  istio_auth       = var.istio_auth
  configs = {
    kiali = {
      ingress   = local.ingress["kiali"]
      http_auth = true
    }
    zipkin = {
      ingress   = local.ingress["zipkin"]
      http_auth = true
    }
  }
}
*/

module "s3" {
  source           = "./modules/s3"
  folder_id = var.yandex_folder_id
  loki_bucket_name = var.loki_bucket_name
  s3_service_account = var.s3_service_account
  s3_service_account_loki = var.s3_service_account_loki
}

/*
module "sentry" {
  source           = "./modules/sentry"
  node_selector    = local.node_selectors["service"]
  cluster_domain   = var.cluster_domain
  pg_sentry_user_name       = var.pg_sentry_user_name
  pg_sentry_user_password   = var.pg_sentry_user_password
  sentry_username = var.sentry_username
  sentry_password = var.sentry_password
  pg_host = var.pg_host
  pg_port = var.pg_port
  enable_sentry = var.enable_sentry
  configs = {
    sentry = {
      ingress   = local.ingress["sentry"]
      http_auth = false
    }
  }
  dep            = [
    module.cert-manager.cluster_issuers["production"],
    module.istio,
    module.nginx-ingress,
  ]
}

module "centrifugo" {
  source = "./modules/centrifugo"

  configs = {
    centrifugo = {
      ingress       = local.ingress["centrifugo"]
      node_selector = local.node_selectors["service"]
      http_auth = false
    }
  }

  centrifugo_replicaCount = var.centrifugo_replicaCount
  redis_password = module.redis.redis_password
  centrifugo_admin_password = var.centrifugo_admin_password
  redis_centrifugo_host = var.redis_centrifugo_host
  centrifugo_admin_secret = var.centrifugo_admin_secret
  centrifugo_api_key = var.centrifugo_api_key

  dep            = [
    module.cert-manager.cluster_issuers["production"],
    module.istio,
    module.nginx-ingress,
  ]
}

module "service-account" {
  source = "./modules/service-account"
}
*/
