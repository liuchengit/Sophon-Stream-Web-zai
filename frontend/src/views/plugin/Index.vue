<template>
  <div class="plugin-page">
    <PageHeader title="插件管理" description="管理系统功能插件，支持动态加载和卸载">
      <template #actions>
        <el-button type="primary" :icon="Plus" @click="installDialogVisible = true">安装插件</el-button>
      </template>
    </PageHeader>

    <el-row :gutter="16">
      <el-col :xs="24" :sm="12" :md="8" v-for="plugin in pluginStore.plugins" :key="plugin.id">
        <el-card shadow="never" class="plugin-card">
          <div class="plugin-header">
            <div class="plugin-icon">
              <el-icon :size="28"><Connection /></el-icon>
            </div>
            <div class="plugin-title-area">
              <h3 class="plugin-name">{{ plugin.name }}</h3>
              <span class="plugin-version">v{{ plugin.version }}</span>
            </div>
            <el-switch
              :model-value="plugin.status === 'active'"
              @change="(val: boolean) => handleToggle(plugin, val)"
              active-color="#67C23A"
            />
          </div>
          <div class="plugin-meta">
            <el-tag size="small" type="info">{{ plugin.type }}</el-tag>
            <StatusBadge :type="plugin.status === 'active' ? 'active' : 'inactive'" size="small" />
          </div>
          <p class="plugin-desc">{{ plugin.description || '暂无描述' }}</p>
          <div class="plugin-actions">
            <el-button text type="primary" size="small" @click="showConfig(plugin)">配置</el-button>
            <el-button text type="danger" size="small" @click="handleDelete(plugin)">卸载</el-button>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <el-empty v-if="pluginStore.plugins.length === 0 && !pluginStore.loading" description="暂无已安装插件" />

    <!-- Install Dialog -->
    <el-dialog v-model="installDialogVisible" title="安装插件" width="480px" :close-on-click-modal="false">
      <el-form :model="installForm" label-width="80px">
        <el-form-item label="插件路径">
          <el-input v-model="installForm.path" placeholder="/path/to/plugin.so 或上传文件" />
        </el-form-item>
        <el-form-item label="说明">
          <el-text type="info" size="small">请输入插件的 .so 文件路径，系统将自动验证并加载</el-text>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="installDialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="installing" @click="handleInstall">安装</el-button>
      </template>
    </el-dialog>

    <!-- Config Dialog -->
    <el-dialog v-model="configDialogVisible" :title="`插件配置 - ${configPlugin?.name || ''}`" width="600px" :close-on-click-modal="false">
      <JsonEditor v-model="pluginConfig" title="插件配置" :rows="12" />
      <template #footer>
        <el-button @click="configDialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="configSaving" @click="handleSaveConfig">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Plus } from '@element-plus/icons-vue'
import { usePluginStore } from '@/stores/plugin'
import PageHeader from '@/components/common/PageHeader.vue'
import StatusBadge from '@/components/common/StatusBadge.vue'
import JsonEditor from '@/components/common/JsonEditor.vue'
import type { Plugin } from '@/api/plugin'

const pluginStore = usePluginStore()
const installDialogVisible = ref(false)
const installing = ref(false)
const configDialogVisible = ref(false)
const configSaving = ref(false)
const configPlugin = ref<Plugin | null>(null)
const pluginConfig = ref<Record<string, any>>({})

const installForm = reactive({
  path: '',
})

async function handleToggle(plugin: Plugin, active: boolean) {
  try {
    if (active) {
      await pluginStore.activatePlugin(plugin.id)
      ElMessage.success(`插件 ${plugin.name} 已激活`)
    } else {
      await pluginStore.deactivatePlugin(plugin.id)
      ElMessage.success(`插件 ${plugin.name} 已停用`)
    }
  } catch { /* handled */ }
}

async function handleInstall() {
  if (!installForm.path) {
    ElMessage.warning('请输入插件路径')
    return
  }
  installing.value = true
  try {
    await pluginStore.installPlugin({ path: installForm.path })
    ElMessage.success('插件安装成功')
    installDialogVisible.value = false
    installForm.path = ''
  } finally {
    installing.value = false
  }
}

function showConfig(plugin: Plugin) {
  configPlugin.value = plugin
  pluginConfig.value = { ...plugin.config }
  configDialogVisible.value = true
}

async function handleSaveConfig() {
  if (!configPlugin.value) return
  configSaving.value = true
  try {
    await pluginStore.updatePlugin(configPlugin.value.id, { config: pluginConfig.value })
    ElMessage.success('配置已保存')
    configDialogVisible.value = false
  } finally {
    configSaving.value = false
  }
}

async function handleDelete(plugin: Plugin) {
  try {
    await ElMessageBox.confirm(`确定要卸载插件 ${plugin.name} 吗？`, '卸载确认', { type: 'danger' })
    await pluginStore.deletePlugin(plugin.id)
    ElMessage.success('插件已卸载')
  } catch { /* cancelled */ }
}

onMounted(() => {
  pluginStore.fetchPlugins()
})
</script>

<style scoped lang="scss">
.plugin-card {
  background: #132f4c;
  border: 1px solid #1e3a5f;
  margin-bottom: 16px;
  transition: all 0.3s;

  &:hover {
    border-color: #409EFF;
    box-shadow: 0 4px 16px rgba(64, 158, 255, 0.1);
  }

  :deep(.el-card__body) {
    padding: 16px;
  }
}

.plugin-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
}

.plugin-icon {
  width: 44px;
  height: 44px;
  border-radius: 10px;
  background: rgba(64, 158, 255, 0.15);
  color: #409EFF;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.plugin-title-area {
  flex: 1;
}

.plugin-name {
  font-size: 15px;
  font-weight: 600;
  color: #e3e8ef;
  margin: 0;
}

.plugin-version {
  font-size: 12px;
  color: #78909c;
}

.plugin-meta {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.plugin-desc {
  font-size: 13px;
  color: #b0bec5;
  line-height: 1.5;
  margin-bottom: 12px;
  min-height: 40px;
}

.plugin-actions {
  display: flex;
  justify-content: flex-end;
  gap: 4px;
  border-top: 1px solid #1e3a5f;
  padding-top: 12px;
}
</style>
