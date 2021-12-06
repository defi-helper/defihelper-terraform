module "main" {
    source              = "../../"
    yandex_token        = var.yandex_token
    cluster_name        = "internetuniversitet"
    yandex_folder_id    = "b1grk3rdup6qhidhb34n"
    yandex_cloud_id     = "b1gdvokdga6o1m21uekf"
    cluster_domain      = "iu1.ru"
    admin_email         = "admin@1iu.ru"
    kubeconfig_path     = "${path.root}/kubeconfig_development.yaml"
    node_groups_scale  = {
        service = {
            fixed_scale = 2
        }
        nfs = {
            fixed_scale = 1
        }
        web = {
            fixed_scale = 1
        }
    }
    admins = {
        mastyf = {
            public_keys = ["ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/z/b1IcwZu3ERCw5Xg99u0VbNR5ZlR4htZ88ANfq2VOjoXffo3290lTNuALGWzYV6A/l7ZW7K/93pmCcZL905T/So6YQlxhMcPmksQUMS7KGCSXrofEmm2+aPA3xnhXiSqrVCwCc0wPdfGlnNeaV4b2gYkyGIwSWv0It6YfJqooulwddHUU3Ux2NnoD3A1D9xpykfkiK/JsR4UzjhWNoswntHlwpuqccdBSMMzSick/AHbzy6rWfFsNNbtnRYtiXMuGbfMRyjZCm50sOUNxHpNRlSzb+xf/oMtqRR7BZ6mbEQAc7PVxPzgkBti4jUqb5YvZ3eNe8uIgkb2T9MBb9r vk@vkmac.local"]
        }
        eyumorozov = {
            public_keys = ["ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQCozelxvZcpHiE0mhxj0ZujjzF37MnQwazw0roZQOnjVCwKumEfwCzwB1ZdBlaagYiu1KcV2NbGN5C2GG1toPf2k6kJfPl5u6Drl6JIO2QanyxEjX/shrxtx2dYwooF88uXN9YDApKtzPyN/pWTVsrwM40xpImvpzW5Gx0OC4U/KZHGM60lPhUIq55fa2iVUSflSp8kPT7tZkYsRivyOVZQJn/YQJdNYpf8beNPs9XBu1C/goU/7KMkVKjVnjA+t853f62A6UGRtUnR3uU4T0JmlHK6x0WVYaw2IXA7HvtC30SZMLL7gvheiTKeVnTCWFzlO62Re/0SrzJNsJjUNhXkHaQPdwmsA+dzHQfaRwvvpeZ6fHf5xs095+zOBZgAKsMKlhVSJkLOFe4aEhOb66Pa3zgoY6709sI/0ffQesV8T3DaYEJAVLOwk3evl9xtGzymqCgz8hwpTJjhOhgFk5FgBSIKsfEvLswocDwcg/LZtlRCoYfh5EhGeynVu1azSdB5fR0YmvkUhkdQu1eDJyia3IcjCNvnB3NV5HTX5gHvoYZ3tSsT5UZNuk4iCTHS2CqaoqL5Zf8qp/YCjL+nr+Bz9yoXjumNJ5yDD/LuiVb4qZiQfkFMWC69FWr2ec2BpMVw9SPSH4YUyv+rIa574/dTTVj46AbZPszXduOZ2V221w== eyumorozov.working@gmail.com"]
        }
    }

    bastion_nat_ip_address              = "178.154.205.31"
    bastion_core_fractions              = 5
    bastion_cores                       = 2
    bastion_memory                      = 2
    bastion_allow_stopping_for_update   = false
}