variable "cluster_id" {
  type = string
}
variable "kube_version" {
  type = string
  default = "1.20"
}
variable "location_subnets" {
  type = list(object({
    id = string
    zone = string
  }))
}
variable "cluster_node_groups" {
  type = map(object({
    name = string
    cpu = number
    memory = number
    core_fraction = number
    disk = object({
      size = number
      type = string
    })
    fixed_scale = list(number)
    auto_scale = list(object({
      max = number
      min = number
      initial = number
    }))
    regional = bool
    node_group_label = string
    node_group_taint = list(string)
    subnet_num = number
  }))
}
variable "ssh_keys" {
  type = string
}
