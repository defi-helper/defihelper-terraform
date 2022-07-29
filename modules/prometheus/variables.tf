variable "configs" {}

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

variable "monitoring_sa_api_key" {
  type = string
}

variable "folder_id" {
  type = string
}

variable "postgres_id" {
  type = string
}

variable "redis_id" {
  type = string
}

variable "pg_defihelper_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_scanner_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_adapters_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_open_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_bctrader_user_conn_limit" {
  description = ""
  default     = ""
}
variable "pg_ba_user_conn_limit" {
  description = ""
  default     = ""
}
