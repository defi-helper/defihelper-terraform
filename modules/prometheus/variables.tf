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
/*
variable "grafana_gitlab_application_id" {
  type = string
}

variable "grafana_gitlab_secret" {
  type = string
}

variable "telegram_bot_admins" {
  type = string
}
*/
variable "telegram_token" {
  type = string
}

variable "telegram_chat_id" {
  type = string
}