variable "configs" {}

variable "cluster_domain" {
  type = string
}
variable "node_selector" {
  type = map(string)
}
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

variable "pg_sentry_user_name" {
    type = string
}

variable "pg_sentry_user_password" {
    type = string
}

variable "enable_sentry" {
  type = string
}
