variable "configs" {}

variable "redis_centrifugo_host" {
    type = string
}
variable "redis_password" {
    type = string
}
variable "centrifugo_admin_password" {
    type = string
}
variable "centrifugo_replicaCount" {
    type = number
}
variable "centrifugo_admin_secret" {
    type = string
}
variable "centrifugo_api_key" {
    type = string
}
