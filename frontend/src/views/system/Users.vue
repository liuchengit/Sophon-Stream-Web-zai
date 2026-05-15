<template>
  <div class="users-page">
    <PageHeader title="用户管理" description="管理系统用户和角色权限">
      <template #actions>
        <el-button type="primary" :icon="Plus" @click="showForm(null)">添加用户</el-button>
      </template>
    </PageHeader>

    <el-card shadow="never" class="table-card">
      <el-table :data="users" v-loading="loading" stripe>
        <el-table-column prop="id" label="ID" width="60" />
        <el-table-column prop="username" label="用户名" width="140" />
        <el-table-column prop="role" label="角色" width="120">
          <template #default="{ row }">
            <el-tag :type="roleTagMap[row.role] || 'info'" size="small">{{ roleLabel[row.role] || row.role }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200">
          <template #default="{ row }">
            <el-button text type="primary" size="small" @click="showForm(row)">编辑</el-button>
            <el-button text type="danger" size="small" @click="handleDelete(row)" :disabled="row.role === 'admin'">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- User Form Dialog -->
    <el-dialog v-model="formVisible" :title="editingUser ? '编辑用户' : '添加用户'" width="440px" :close-on-click-modal="false">
      <el-form ref="formRef" :model="userForm" :rules="formRules" label-width="70px">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="userForm.username" placeholder="请输入用户名" :disabled="!!editingUser" />
        </el-form-item>
        <el-form-item v-if="!editingUser" label="密码" prop="password">
          <el-input v-model="userForm.password" type="password" show-password placeholder="请输入密码" />
        </el-form-item>
        <el-form-item label="角色" prop="role">
          <el-select v-model="userForm.role" style="width: 100%">
            <el-option label="管理员" value="admin" />
            <el-option label="操作员" value="operator" />
            <el-option label="查看者" value="viewer" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="formVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSave">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'
import { getUsers, register, updateUser, deleteUser } from '@/api/auth'
import type { UserInfo } from '@/api/auth'
import PageHeader from '@/components/common/PageHeader.vue'

const users = ref<UserInfo[]>([])
const loading = ref(false)
const formVisible = ref(false)
const editingUser = ref<UserInfo | null>(null)
const formRef = ref<FormInstance>()
const saving = ref(false)

const roleTagMap: Record<string, string> = { admin: 'danger', operator: 'warning', viewer: 'info' }
const roleLabel: Record<string, string> = { admin: '管理员', operator: '操作员', viewer: '查看者' }

const userForm = reactive({
  username: '',
  password: '',
  role: 'viewer',
})

const formRules: FormRules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, message: '请输入密码', trigger: 'blur' }, { min: 6, message: '密码至少6位', trigger: 'blur' }],
  role: [{ required: true, message: '请选择角色', trigger: 'change' }],
}

function showForm(user: UserInfo | null) {
  editingUser.value = user
  if (user) {
    userForm.username = user.username
    userForm.role = user.role
    userForm.password = ''
  } else {
    userForm.username = ''
    userForm.password = ''
    userForm.role = 'viewer'
  }
  formVisible.value = true
}

async function handleSave() {
  const formEl = formRef.value
  if (!formEl) return
  await formEl.validate()
  saving.value = true
  try {
    if (editingUser.value) {
      await updateUser(editingUser.value.id, { role: userForm.role })
      ElMessage.success('用户更新成功')
    } else {
      await register({ username: userForm.username, password: userForm.password, role: userForm.role })
      ElMessage.success('用户添加成功')
    }
    formVisible.value = false
    loadUsers()
  } finally {
    saving.value = false
  }
}

async function handleDelete(user: UserInfo) {
  try {
    await ElMessageBox.confirm(`确定要删除用户 ${user.username} 吗？`, '删除确认', { type: 'danger' })
    await deleteUser(user.id)
    ElMessage.success('用户已删除')
    loadUsers()
  } catch { /* cancelled */ }
}

async function loadUsers() {
  loading.value = true
  try {
    users.value = await getUsers()
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  loadUsers()
})
</script>

<style scoped lang="scss">
.table-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
}
</style>
