variable "yandex_token" {
  type = string
}
variable "yandex_cloud_id" {
  type = string
}
variable "yandex_folder_id" {
  type = string
}
variable "cluster_name" {
  type = string
}
variable "cluster_version" {
  type    = string
  default = "1.22"
}
variable "cluster_release_channel" {
  type    = string
  default = "STABLE"
}
variable "node_groups_scale" {
  default = {
    service = {
      fixed_scale = 3
      regional = false
      node_group_label = "service"
      subnet_num = 0
    }
    nfs = {
      fixed_scale = 1
      regional = false
      node_group_label = "nfs"
      subnet_num = 0
    }
    web = {
      auto_scale = {
        max     = 3
        min     = 3
        initial = 3
      }
      regional = false
      node_group_label = "web"
      subnet_num = 0
    }
  }
}
variable "cluster_node_group_configs" {
  default = {
    service = {
      name          = "service"
      cpu           = 2
      memory        = 8
      core_fraction = 20
      disk = {
        size = 64
        type = "network-ssd"
      }
    }
    nfs = {
      name          = "nfs"
      cpu           = 2
      memory        = 2
      core_fraction = 5
      disk = {
        size = 64
        type = "network-ssd"
      }
    }
    web = {
      name          = "web"
      cpu           = 2
      memory        = 4
      core_fraction = 5
      disk = {
        size = 64
        type = "network-ssd"
      }
    }
  }
}

variable "load_balancer_ip" {
  type = string
}
variable "nginx_ingress_replicacount" {
  type = number
}
variable "nginx_ingress_backend_replicacount" {
  type = number
}

variable "nginx_ingress_replicacount_max" {
  type = number
}

variable "nginx_ingress_cpu_request" {
  type = string
}

variable "nginx_ingress_memory_request" {
  type = string
}
variable "admin_email" {
  type = string
}
variable "cluster_domain" {
  type = string
}
variable "admins" {
  type = map(object({
    public_keys = list(string)
  }))
}
variable "output_dir" {
  type    = string
  default = "output"
}

/*
variable "kubeconfig_path" {
  type = string
}
*/
###################################### Bastion ######################################

variable "bastion_allow_stopping_for_update" {
  description = ""
  default     = ""
}

variable "bastion_vm_image" {
  description = ""
  default     = ""
}

variable "bastion_core_fractions" {
  description = ""
  default     = ""
}

variable "bastion_cores" {
  description = ""
  default     = ""
}

variable "bastion_memory" {
  description = ""
  default     = ""
}

variable "bastion_nat_ip_address" {
  description = ""
  default     = ""
}

variable "bastion_ssh_users_file_path" {
  description = ""
  default     = ""
}

###################################### Bastion End ######################################

###################################### hosting ######################################
variable "enable_hosting" {
  description = ""
  default     = ""
}

variable "hosting_vm_image" {
  description = ""
  default     = ""
}

variable "hosting_core_fractions" {
  description = ""
  default     = ""
}

variable "hosting_disk_size" {
  description = ""
  default     = ""
}

variable "hosting_cores" {
  description = ""
  default     = ""
}

variable "hosting_memory" {
  description = ""
  default     = ""
}

variable "hosting_nat_ip_address" {
  description = ""
  default     = ""
}

variable "hosting_ssh_users_file_path" {
  description = ""
  default     = ""
}

###################################### Hosting End ######################################

###################################### Managed Database ######################################
variable "pg_version" {
  description = ""
  default     = ""
}
variable "pg_tier_id" {
  description = ""
  default     = ""
}
variable "pg_instance_disk_size" {
  description = ""
  default     = ""
}
variable "pg_admin_name" {
  description = ""
  default     = ""
}
variable "pg_admin_password" {
  description = ""
  default     = ""
}
variable "pg_admin_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_public_ip" {
  description = ""
  default     = false
}
variable "enable_replication" {
  description = ""
  default     = ""
}
variable "location_subnets" {
  description = ""
  default     = ""
}
variable "pg_defihelper_user_name" {
  description = ""
  default     = ""
}
variable "pg_defihelper_user_password" {
  description = ""
  default     = ""
}
variable "pg_defihelper_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_scanner_user_name" {
  description = ""
  default     = ""
}
variable "pg_scanner_user_password" {
  description = ""
  default     = ""
}
variable "pg_scanner_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_open_user_name" {
  description = ""
  default     = ""
}
variable "pg_open_user_password" {
  description = ""
  default     = ""
}
variable "pg_open_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_seeker_user_name" {
  description = ""
  default     = ""
}
variable "pg_seeker_user_password" {
  description = ""
  default     = ""
}
variable "pg_seeker_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_ranking_user_name" {
  description = ""
  default     = ""
}
variable "pg_ranking_user_password" {
  description = ""
  default     = ""
}
variable "pg_ranking_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_bctrader_user_name" {
  description = ""
  default     = ""
}
variable "pg_bctrader_user_password" {
  description = ""
  default     = ""
}
variable "pg_bctrader_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_ba_user_name" {
  description = ""
  default     = ""
}
variable "pg_ba_user_password" {
  description = ""
  default     = ""
}
variable "pg_ba_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_adapters_user_name" {
  description = ""
  default     = ""
}
variable "pg_adapters_user_password" {
  description = ""
  default     = ""
}
variable "pg_adapters_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_dev01_user_name" {
  description = ""
  default     = ""
}
variable "pg_dev01_user_password" {
  description = ""
  default     = ""
}
variable "pg_dev01_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_dev02_user_name" {
  description = ""
  default     = ""
}
variable "pg_dev02_user_password" {
  description = ""
  default     = ""
}
variable "pg_dev02_user_conn_limit" {
  description = ""
  default     = ""
}
###################################### Managed Database End ######################################
/*
###################################### Gitlab ######################################
variable "gitlab_certificate_secret_name" {
  type = string
}
variable "ssl_1iu_ru_crt" {
  type = string
}
variable "ssl_1iu_ru_key" {
  type = string
}
variable "gitlab_psql_host" {
  type = string
}
variable "gitlab_redis_host" {
  type = string
}
variable "gitlab_redis_port" {
  type = string
}
variable "gitlab_email_from" {
  type = string
}
variable "gitlab_smtp_address" {
  type = string
}
variable "gitlab_smtp_port" {
  type = string
}
variable "gitlab_psql_port" {
  type = string
}
variable "gitlabstorage-static-key" {
  type = string
}
variable "backups_cron_git" {
  type = string
}
variable "gitlab_smtp_user_name" {
  type = string
}
variable "gitlab_smtp_password" {
  type = string
}
variable "git-s3cfg" {
  type = string
}
variable "gitlab-backup-storage" {
  type = string
}
variable "gitlab-tmp-storage" {
  type = string
}
variable "version_gitlab" {
  type = string
}
variable "enable_gitlab" {
  type = string
}
*/

###################################### RabbitMQ ######################################
variable "rabbitmq_host" {
  type = string
}
variable "rabbitmq_password" {
  type = string
}
variable "rabbitmq_erlangcookie" {
  type = string
}
variable "rabbitmq_replicaCount" {
  type = number
}

###################################### pgadmin4 ######################################
variable "pgadmin4_admin_password" {
  type = string
}
variable "pgadmin4_domain" {
  type = string
}
variable "pgadmin4_admin_email" {
  type = string
}

###################################### gitlab-runner ######################################
variable "gitlabRunnerRegistrationToken" {
  type = string
}
variable "enable_gitlab_runner" {
  type = string
}
variable "gitlab_runner_docker_io_auth" {
  type = string
}
variable "gitlab_runner_tags" {
  type = string
}

###################################### kube-prometheus stack ######################################

variable "grafana_admin_password" {
  type = string
}

variable "prometheus_auth" {
  type = string
}

variable "alertmanager_email_from" {
  type = string
}

variable "alertmanager_email_to" {
  type = string
}

variable "alertmanager_smtp_address" {
  type = string
}

variable "alertmanager_smtp_password" {
  type = string
}

variable "grafana_gitlab_application_id" {
  type = string
}

variable "grafana_gitlab_secret" {
  type = string
}

variable "telegram_bot_admins" {
  type = string
}

variable "telegram_token" {
  type = string
}

variable "telegram_chat_id" {
  type = string
}

/*
###################################### nfs-provisioner ######################################

variable "nfs_disk_size" {
  type = string
}

variable "dns_zones" {
  description = ""
  default     = ""
}
variable "dns_zones_rs" {
  description = ""
  default     = ""
}
##################################### Istio ##################################################
variable "istio_auth" {
    type = string
}
*/
##################################### Loki ################################################
variable "loki_bucket_name" {
    type = string
}
variable "open_bucket_name" {
  type = string
}

variable "s3_service_account" {
    type = string
}

variable "s3_service_account_loki" {
    type = string
}

/*
###################################### Sentry #############################################
variable "sentry_username" {
    type = string
}

variable "sentry_password" {
    type = string
}

variable "pg_host" {
    type = string
}

variable "pg_port" {
    type = string
}
variable "enable_sentry" {
  type = string
}

#################################### Centrifugo ###########################################
variable "redis_centrifugo_host" {
    type = string
}
variable "centrifugo_replicaCount" {
    type = number
}
variable "centrifugo_host" {
    type = string
}
variable "centrifugo_admin_password" {
    type = string
}
variable "centrifugo_admin_secret" {
    type = string
}
variable "centrifugo_api_key" {
    type = string
}
*/

#################################### Redis ###########################################
variable "redis_host_class" {
  type = string
}

###################################### node-local-dns #############################################
variable "enable_node_local_dns" {
  type = string
}
