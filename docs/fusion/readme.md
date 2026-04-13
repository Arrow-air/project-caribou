# Fusion 360 Workspace — Project Caribou

## Folder Structure

```
📁 Caribou_<Name>        — Personal sandbox (each contributor creates their own)
📁 Deprecated             — Move obsolete files here (do NOT delete)
📁 Main_Assembly          — Master Assembly ONLY (no parts, no features)
📁 Parts/
   ├── EL-Electronics
   ├── EN-Enclosure
   ├── PW-Powertrain
   └── ST-Structure
📁 Sub_Assemblies         — Sub-assemblies composed of parts or other sub-assemblies
```

## Naming Convention

### Parts

Format: `<Category>-<Type>-<Number>-<Description>`

| Example | Meaning |
|---|---|
| `ST-FRM-001-Spacer_1` | Structure — Frame — Part 001 |
| `PW-MTR-001-Motor_Mount` | Powertrain — Motor — Part 001 |
| `EL-PCB-001-BMS_Enclosure` | Electronics — PCB — Part 001 |

### Sub-Assemblies

Format: `<Category>-<Description>_Assembly`

| Example | Meaning |
|---|---|
| `PW-Propulsion_System_Assembly` | Powertrain sub-assembly |
| `ST-Frame_Assembly` | Structure sub-assembly |

### Category Prefixes

| Prefix | Category |
|---|---|
| `ST` | Structure |
| `PW` | Powertrain |
| `EL` | Electronics |
| `EN` | Enclosure |

## Workflow

### Strict Rules

1. **No feature creation in assemblies.** Use Fusion's file type setting — choose either `Assembly` or `Part Design`. **Never use Hybrid.**
2. **Main_Assembly = Assembly only.** Only joints, alignment, and mirroring. No parts are designed here.
3. **Working Assembly Method:**
   - Create a separate assembly file in your personal folder (`Caribou_<Name>`)
   - Insert the parts/sub-assemblies you need from Main_Assembly
   - Do all design work in dedicated Part files under `Parts/`
   - Once finished, insert your part/sub-assembly back into Main_Assembly

### General Workflow

1. **New part?** → Create it in the correct `Parts/<Category>` folder using the naming convention
2. **Testing/iterating?** → Work in your `Caribou_<Name>` sandbox
3. **Part is done?** → Insert into the relevant Sub-Assembly or Main_Assembly
4. **Part is obsolete?** → Move to `Deprecated/` — never delete

## Access

- All contributors get **Edit** access to the Fusion 360 project
- Main_Assembly changes should be communicated in **#caribou-team** before pushing
